#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <inputtino/input.hpp>
#include <mutex>
#include <udev_helpers.hpp>
#include <uhid/protected_types.hpp>
#include <uhid/switch.hpp>
#include <uhid/uhid.hpp>

namespace inputtino {

namespace {

// Real Bluetooth Pro Controller 0x30 reports on the host consistently show:
// - bat_con = 0x60
// - button_status = 00 80 00 at idle
//
// Mirror that steady-state shape as closely as possible so hid-nintendo sees
// reports that look like a real controller rather than a synthetic
// "full+charging" device with an all-zero button prefix.
constexpr uint8_t SWITCH_BATTERY_HIGH_BT = 0x60;
constexpr uint8_t SWITCH_BUTTON_STATUS1_BASE = 0x80;

// Map signed 16-bit stick axis to 12-bit Switch range [0, 4095] centered at 2048.
int scale_axis(short value) {
  auto slope = static_cast<double>(uhid::SWITCH_AXIS_MAX) / 65535.0;
  return static_cast<int>(std::round(slope * (static_cast<int>(value) + 32768)));
}

// Pack two 12-bit values into 3 bytes: X in bits 0-11, Y in bits 12-23.
void pack_12bit_pair(uint8_t *out, uint16_t x, uint16_t y) {
  out[0] = static_cast<uint8_t>(x & 0xFF);
  out[1] = static_cast<uint8_t>(((x >> 8) & 0x0F) | ((y & 0x0F) << 4));
  out[2] = static_cast<uint8_t>((y >> 4) & 0xFF);
}

// Left stick factory calibration at 0x603D: hid-nintendo parses as
// (max_above, center, min_below).
void fill_left_stick_calibration(uint8_t *data) {
  pack_12bit_pair(data + 0, 2047, 2047); // max_above
  pack_12bit_pair(data + 3, 2048, 2048); // center
  pack_12bit_pair(data + 6, 2048, 2048); // min_below
}

// Right stick factory calibration at 0x6046: hid-nintendo parses as
// (center, min_below, max_above) — different order from left stick.
void fill_right_stick_calibration(uint8_t *data) {
  pack_12bit_pair(data + 0, 2048, 2048); // center
  pack_12bit_pair(data + 3, 2048, 2048); // min_below
  pack_12bit_pair(data + 6, 2047, 2047); // max_above
}

// IMU factory calibration layout (24 bytes, matching hid-nintendo's parsing):
//   bytes  0-5:  accel offset  (3x int16 LE)
//   bytes  6-11: accel scale   (3x int16 LE)
//   bytes 12-17: gyro offset   (3x int16 LE)
//   bytes 18-23: gyro scale    (3x int16 LE)
// The kernel computes divisor = scale - offset per axis.
void fill_imu_calibration(uint8_t *data) {
  auto write_le16 = [&](int index, int16_t value) {
    data[index] = static_cast<uint8_t>(value & 0xFF);
    data[index + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
  };

  for (int i = 0; i < 3; ++i) {
    write_le16(i * 2, uhid::SWITCH_IMU_ACCEL_OFFSET);
    write_le16(6 + i * 2, uhid::SWITCH_IMU_ACCEL_SCALE_CAL);
    write_le16(12 + i * 2, uhid::SWITCH_IMU_GYRO_OFFSET);
    write_le16(18 + i * 2, uhid::SWITCH_IMU_GYRO_SCALE_CAL);
  }
}

int decode_rumble_high_amplitude(uint8_t encoded) {
  // encoded ∈ [0x00, 0xC8] maps linearly to amplitude ∈ [0, 65535]
  return static_cast<int>(std::lround(encoded * (65535.0 / 0xC8)));
}

int decode_rumble_low_amplitude(uint16_t encoded) {
  // Bits 7:0 range from 0x40 to 0x72 (→ base index 0..50); bit 15 is a half-step flag.
  // Combined index ∈ [0, 100] maps linearly to amplitude ∈ [0, 65535].
  int index = (static_cast<int>(encoded & 0xFF) - 0x40) * 2 + ((encoded >> 15) & 1);
  return static_cast<int>(std::lround(std::clamp(index, 0, 100) * (65535.0 / 100)));
}

std::pair<int, int> decode_rumble_block(const uhid::switch_rumble_data &rumble) {
  auto high_amplitude = decode_rumble_high_amplitude(rumble.amp_high);
  auto low_amplitude =
      decode_rumble_low_amplitude(static_cast<uint16_t>((rumble.freq_low & 0x80) << 8) | rumble.amp_low);
  return {low_amplitude, high_amplitude};
}

void fill_reply_prefix(SwitchJoypadState &state, uhid::switch_standard_input_prefix &prefix, uint8_t report_id) {
  prefix.report_id = report_id;
  prefix.timer = state.timer++;
  prefix.bat_con = SWITCH_BATTERY_HIGH_BT;
  prefix.vibrator_input = 0;
  std::copy(std::begin(state.buttons), std::end(state.buttons), std::begin(prefix.button_status));
  pack_12bit_pair(prefix.left_stick, state.lx, state.ly);
  pack_12bit_pair(prefix.right_stick, state.rx, state.ry);
}

void send_report(SwitchJoypadState &state) {
  std::lock_guard<std::mutex> lock(state.mtx);

  if (state.report_mode != uhid::SWITCH_REPORT_MODE_STANDARD_FULL) {
    return;
  }

  uhid::switch_input_report report{};
  fill_reply_prefix(state, report.prefix, uhid::SWITCH_INPUT_REPORT_STANDARD_FULL);
  std::copy(std::begin(state.imu), std::end(state.imu), std::begin(report.imu));

  struct uhid_event ev {};
  ev.type = UHID_INPUT2;
  std::copy(reinterpret_cast<unsigned char *>(&report),
            reinterpret_cast<unsigned char *>(&report) + sizeof(report),
            &ev.u.input2.data[0]);
  ev.u.input2.size = sizeof(report);
  if (state.dev) {
    state.dev->send(ev);
  }
}

void send_subcmd_reply(
    SwitchJoypadState &state, uint8_t ack, uint8_t subcmd_id, const uint8_t *payload, size_t payload_size) {
  std::lock_guard<std::mutex> lock(state.mtx);

  uhid::switch_subcmd_reply_report report{};
  fill_reply_prefix(state, report.prefix, uhid::SWITCH_INPUT_REPORT_SUBCOMMAND_REPLY);
  report.ack = ack;
  report.subcmd_id = subcmd_id;
  if (payload && payload_size > 0) {
    std::copy(payload, payload + std::min(payload_size, sizeof(report.data)), std::begin(report.data));
  }

  struct uhid_event ev {};
  ev.type = UHID_INPUT2;
  std::copy(reinterpret_cast<unsigned char *>(&report),
            reinterpret_cast<unsigned char *>(&report) + sizeof(report),
            &ev.u.input2.data[0]);
  ev.u.input2.size = sizeof(report);
  if (state.dev) {
    state.dev->send(ev);
  }
}

void handle_spi_flash_read(SwitchJoypadState &state, const uint8_t *request_data) {
  uint32_t address = static_cast<uint32_t>(request_data[0]) | (static_cast<uint32_t>(request_data[1]) << 8) |
                     (static_cast<uint32_t>(request_data[2]) << 16) | (static_cast<uint32_t>(request_data[3]) << 24);
  uint8_t size = request_data[4];

  uint8_t payload[35] = {};
  payload[0] = request_data[0];
  payload[1] = request_data[1];
  payload[2] = request_data[2];
  payload[3] = request_data[3];
  payload[4] = size;

  switch (address) {
  case uhid::JC_CAL_USR_LEFT_MAGIC_ADDR:
  case uhid::JC_CAL_USR_RIGHT_MAGIC_ADDR:
  case uhid::JC_IMU_CAL_USR_MAGIC_ADDR:
    payload[5] = 0x00;
    payload[6] = 0x00;
    break;
  case uhid::JC_CAL_FCT_DATA_LEFT_ADDR:
    fill_left_stick_calibration(&payload[5]);
    // SDL reads both sticks in one 18-byte SPI read from 0x603D.
    // The kernel reads them separately (9 bytes each from 0x603D and 0x6046).
    // Fill the right stick data at offset +9 so both paths work.
    if (size >= 18) {
      fill_right_stick_calibration(&payload[5 + 9]);
    }
    break;
  case uhid::JC_CAL_FCT_DATA_RIGHT_ADDR:
    fill_right_stick_calibration(&payload[5]);
    break;
  case uhid::JC_IMU_CAL_FCT_DATA_ADDR:
    fill_imu_calibration(&payload[5]);
    break;
  default:
    break;
  }

  send_subcmd_reply(state,
                    uhid::SWITCH_ACK_SPI_FLASH_READ,
                    uhid::SWITCH_SUBCMD_SPI_FLASH_READ,
                    payload,
                    sizeof(payload));
}

void handle_output_report(std::shared_ptr<SwitchJoypadState> state, const uint8_t *data, size_t size) {
  if (!data || size == 0) {
    return;
  }

  auto report_id = data[0];
  if (report_id == uhid::SWITCH_OUTPUT_REPORT_RUMBLE_ONLY) {
    if (size >= sizeof(uhid::switch_output_report_rumble_only) && state->on_rumble) {
      const auto *report = reinterpret_cast<const uhid::switch_output_report_rumble_only *>(data);
      auto [low_left, high_left] = decode_rumble_block(report->left);
      auto [low_right, high_right] = decode_rumble_block(report->right);
      (*state->on_rumble)(std::max(low_left, low_right), std::max(high_left, high_right));
    }
    return;
  }
  if (report_id != uhid::SWITCH_OUTPUT_REPORT_RUMBLE_AND_SUBCOMMAND) {
    return;
  }
  if (size < sizeof(uhid::switch_output_report_rumble_subcmd)) {
    return;
  }

  const auto *report = reinterpret_cast<const uhid::switch_output_report_rumble_subcmd *>(data);
  auto subcmd_id = report->subcmd_id;
  const uint8_t *subcmd_data = data + sizeof(uhid::switch_output_report_rumble_subcmd);

  switch (subcmd_id) {
  case uhid::SWITCH_SUBCMD_REQ_DEV_INFO: {
    uint8_t payload[35] = {};
    payload[0] = 0x04;
    payload[1] = 0x33;
    payload[2] = state->controller_type;
    payload[3] = 0x02;
    std::copy(state->mac.bytes.begin(), state->mac.bytes.end(), &payload[4]);
    send_subcmd_reply(*state, uhid::SWITCH_ACK_DEV_INFO, subcmd_id, payload, sizeof(payload));
    break;
  }
  case uhid::SWITCH_SUBCMD_SET_INPUT_REPORT_MODE: {
    std::lock_guard<std::mutex> lock(state->mtx);
    state->report_mode = subcmd_data[0];
  }
    send_subcmd_reply(*state, uhid::SWITCH_ACK, subcmd_id, nullptr, 0);
    break;
  case uhid::SWITCH_SUBCMD_ENABLE_IMU: {
    std::lock_guard<std::mutex> lock(state->mtx);
    state->imu_enabled = subcmd_data[0] != 0;
  }
    send_subcmd_reply(*state, uhid::SWITCH_ACK, subcmd_id, nullptr, 0);
    break;
  case uhid::SWITCH_SUBCMD_ENABLE_VIBRATION: {
    std::lock_guard<std::mutex> lock(state->mtx);
    state->vibration_enabled = subcmd_data[0] != 0;
  }
    send_subcmd_reply(*state, uhid::SWITCH_ACK, subcmd_id, nullptr, 0);
    break;
  case uhid::SWITCH_SUBCMD_SPI_FLASH_READ:
    handle_spi_flash_read(*state, subcmd_data);
    break;
  case uhid::SWITCH_SUBCMD_SET_PLAYER_LIGHTS:
    send_subcmd_reply(*state, uhid::SWITCH_ACK, subcmd_id, nullptr, 0);
    break;
  case uhid::SWITCH_SUBCMD_GET_PLAYER_LIGHTS: {
    uint8_t payload[35] = {};
    payload[0] = 0x01;
    send_subcmd_reply(*state, uhid::SWITCH_ACK, subcmd_id, payload, sizeof(payload));
    break;
  }
  case uhid::SWITCH_SUBCMD_SET_HOME_LIGHT:
    send_subcmd_reply(*state, uhid::SWITCH_ACK, subcmd_id, nullptr, 0);
    break;
  default:
    send_subcmd_reply(*state, uhid::SWITCH_ACK, subcmd_id, nullptr, 0);
    break;
  }

  // Subcmd packets also carry rumble data; reset after processing the subcmd
  // so the rumble effect doesn't persist beyond the command exchange.
  if (state->on_rumble) {
    (*state->on_rumble)(0, 0);
  }
}

void on_uhid_event(std::shared_ptr<SwitchJoypadState> state, uhid_event ev, int fd) {
  switch (ev.type) {
  case UHID_OUTPUT:
    handle_output_report(state, ev.u.output.data, ev.u.output.size);
    break;
  case UHID_GET_REPORT: {
    uhid_event answer{};
    answer.type = UHID_GET_REPORT_REPLY;
    answer.u.get_report_reply.id = ev.u.get_report.id;
    answer.u.get_report_reply.err = 0;
    answer.u.get_report_reply.size = 0;
    auto res = uhid::uhid_write(fd, &answer);
    (void)res;
    break;
  }
  case UHID_SET_REPORT: {
    if (ev.u.set_report.rtype == UHID_OUTPUT_REPORT) {
      handle_output_report(state, ev.u.set_report.data, ev.u.set_report.size);
    }

    uhid_event answer{};
    answer.type = UHID_SET_REPORT_REPLY;
    answer.u.set_report_reply.id = ev.u.set_report.id;
    answer.u.set_report_reply.err = 0;
    auto res = uhid::uhid_write(fd, &answer);
    (void)res;
    break;
  }
  default:
    break;
  }
}

} // namespace

SwitchJoypad::SwitchJoypad(const Mac &mac) : _state(std::make_shared<SwitchJoypadState>()) {
  this->_state->mac = mac;
  this->_state->buttons[1] = SWITCH_BUTTON_STATUS1_BASE;
}

SwitchJoypad::~SwitchJoypad() {
  if (this->_state) {
    if (this->_state->dev) {
      this->_state->stop_repeat_thread = true;
      if (this->_send_input_thread.joinable()) {
        this->_send_input_thread.join();
      }
      this->_state->dev->stop_thread();
      this->_state->dev.reset();
    }
  }
}

Result<SwitchJoypad> SwitchJoypad::create(const DeviceDefinition &device) {
  auto mac = device.device_uniq.empty() ? Mac::generate() : Mac::parse(device.device_uniq);
  if (!mac) {
    return Error(mac.getErrorMessage());
  }
  auto joypad = SwitchJoypad(*mac);
  joypad._state->vendor_id = device.vendor_id;
  joypad._state->product_id = device.product_id;

  auto def = uhid::DeviceDefinition{
      .name = device.name,
      .phys = device.device_phys.empty() ? "bluetooth" : device.device_phys,
      .uniq = joypad._state->mac.to_string(),
      .bus = BUS_BLUETOOTH,
      .vendor = static_cast<uint32_t>(device.vendor_id),
      .product = static_cast<uint32_t>(device.product_id),
      .version = static_cast<uint32_t>(device.version),
      .country = 0,
      .report_description = {&uhid::switch_rdesc_bt[0], &uhid::switch_rdesc_bt[0] + sizeof(uhid::switch_rdesc_bt)}};

  auto dev =
      uhid::Device::create(def, [state = joypad._state](uhid_event ev, int fd) { on_uhid_event(state, ev, fd); });
  if (!dev) {
    return Error(dev.getErrorMessage());
  }

  joypad._state->dev = std::make_shared<uhid::Device>(std::move(*dev));
  // Stash the (fully resolved) definition so the device can be re-created in place later — same MAC/uniq, so
  // get_sys_nodes() still matches and lobby hand-off keeps its /dev/input/* layout.
  joypad._state->def = def;
  // A real Pro Controller sends 0x30 reports at ~66Hz (~15ms interval).
  // Mirror that cadence so hid-nintendo's "dropped IMU reports" detection
  // doesn't flag a gap.
  joypad._send_input_thread = uhid_joypad::start_report_pump<SwitchJoypadState>(
      joypad._state,
      [](SwitchJoypadState &s) { send_report(s); },
      std::chrono::milliseconds(15));

  // Only return once the kernel has exposed the device's input nodes in sysfs.
  uhid_joypad::wait_for_sys_nodes([&joypad]() { return joypad.get_sys_nodes(); });
  return joypad;
}
std::string SwitchJoypad::get_mac_address() const {
  return _state->mac.to_string();
}

std::vector<std::string> SwitchJoypad::get_sys_nodes() const {
  return uhid::find_uhid_sys_nodes(_state->vendor_id, _state->mac);
}

std::vector<std::string> SwitchJoypad::get_nodes() const {
  return uhid::sys_nodes_to_dev_paths(get_sys_nodes());
}

void SwitchJoypad::set_pressed_buttons(unsigned int pressed) {
  {
    std::lock_guard<std::mutex> lock(this->_state->mtx);
    auto &buttons = this->_state->buttons;
    buttons[0] = 0;
    buttons[1] = SWITCH_BUTTON_STATUS1_BASE;
    buttons[2] = 0;

    // Positional remap so the Joypad API (Xbox naming) stays consistent
    // across backends. Physically, Switch Y is where Xbox X sits (left),
    // Switch X is where Xbox Y sits (top), Switch B is bottom, Switch A is
    // right — so Xbox A→SwitchB, Xbox B→SwitchA, Xbox X→SwitchY,
    // Xbox Y→SwitchX. SDL's gamepad DB then round-trips to the right
    // SDL_CONTROLLER_BUTTON_*.
    if (X & pressed)
      buttons[0] |= 0x01; // Switch Y slot (west / left)
    if (Y & pressed)
      buttons[0] |= 0x02; // Switch X slot (north / top)
    if (A & pressed)
      buttons[0] |= 0x04; // Switch B slot (south / bottom)
    if (B & pressed)
      buttons[0] |= 0x08; // Switch A slot (east / right)
    if (RIGHT_BUTTON & pressed)
      buttons[0] |= 0x40;

    if (BACK & pressed)
      buttons[1] |= 0x01;
    if (START & pressed)
      buttons[1] |= 0x02;
    if (RIGHT_STICK & pressed)
      buttons[1] |= 0x04;
    if (LEFT_STICK & pressed)
      buttons[1] |= 0x08;
    if (HOME & pressed)
      buttons[1] |= 0x10;
    if (MISC_FLAG & pressed)
      buttons[1] |= 0x20;

    if (DPAD_DOWN & pressed)
      buttons[2] |= 0x01;
    if (DPAD_UP & pressed)
      buttons[2] |= 0x02;
    if (DPAD_RIGHT & pressed)
      buttons[2] |= 0x04;
    if (DPAD_LEFT & pressed)
      buttons[2] |= 0x08;
    if (LEFT_BUTTON & pressed)
      buttons[2] |= 0x40;
  }
  send_report(*this->_state);
}

void SwitchJoypad::set_triggers(int16_t left, int16_t right) {
  {
    std::lock_guard<std::mutex> lock(this->_state->mtx);
    auto &buttons = this->_state->buttons;
    if (left > 0) {
      buttons[2] |= 0x80;
    } else {
      buttons[2] &= ~0x80;
    }
    if (right > 0) {
      buttons[0] |= 0x80;
    } else {
      buttons[0] &= ~0x80;
    }
  }
  send_report(*this->_state);
}

void SwitchJoypad::set_stick(Joypad::STICK_POSITION stick_type, short x, short y) {
  auto scaled_x = static_cast<uint16_t>(scale_axis(x));
  auto scaled_y = static_cast<uint16_t>(scale_axis(y));
  {
    std::lock_guard<std::mutex> lock(this->_state->mtx);
    if (stick_type == LS) {
      this->_state->lx = scaled_x;
      this->_state->ly = scaled_y;
    } else {
      this->_state->rx = scaled_x;
      this->_state->ry = scaled_y;
    }
  }
  send_report(*this->_state);
}

// Switch IMU sample write. is_accel selects accelerometer (m/s^2) vs gyroscope
// (deg/s); both take the SDL/Moonlight convention — the Switch path needs no unit
// fix (unlike PS5's rad/s gyro), only the SDL->hid-nintendo coordinate remap.
static void apply_switch_imu(SwitchJoypadState &state, bool is_accel, float x, float y, float z) {
  // Remap from SDL coordinate frame to hid-nintendo's native frame.
  //   SDL_X = -kernel_Y,  SDL_Y = kernel_Z,  SDL_Z = -kernel_X
  // Invert:  kernel_X = -SDL_Z,  kernel_Y = -SDL_X,  kernel_Z = SDL_Y
  float nx = -z;
  float ny = -x;
  float nz = y;

  int16_t sx = 0;
  int16_t sy = 0;
  int16_t sz = 0;

  if (is_accel) {
    sx = static_cast<int16_t>(
        std::clamp(std::lround((nx / uhid::SWITCH_STANDARD_GRAVITY_CONST) * uhid::SWITCH_ACCEL_SCALE),
                   static_cast<long>(INT16_MIN),
                   static_cast<long>(INT16_MAX)));
    sy = static_cast<int16_t>(
        std::clamp(std::lround((ny / uhid::SWITCH_STANDARD_GRAVITY_CONST) * uhid::SWITCH_ACCEL_SCALE),
                   static_cast<long>(INT16_MIN),
                   static_cast<long>(INT16_MAX)));
    sz = static_cast<int16_t>(
        std::clamp(std::lround((nz / uhid::SWITCH_STANDARD_GRAVITY_CONST) * uhid::SWITCH_ACCEL_SCALE),
                   static_cast<long>(INT16_MIN),
                   static_cast<long>(INT16_MAX)));
  } else {
    sx = static_cast<int16_t>(std::clamp(std::lround(nx * uhid::SWITCH_GYRO_SCALE),
                                         static_cast<long>(INT16_MIN),
                                         static_cast<long>(INT16_MAX)));
    sy = static_cast<int16_t>(std::clamp(std::lround(ny * uhid::SWITCH_GYRO_SCALE),
                                         static_cast<long>(INT16_MIN),
                                         static_cast<long>(INT16_MAX)));
    sz = static_cast<int16_t>(std::clamp(std::lround(nz * uhid::SWITCH_GYRO_SCALE),
                                         static_cast<long>(INT16_MIN),
                                         static_cast<long>(INT16_MAX)));
  }

  {
    std::lock_guard<std::mutex> lock(state.mtx);
    if (!state.imu_enabled) {
      return;
    }
    for (auto &sample : state.imu) {
      if (is_accel) {
        sample.accel[0] = sx;
        sample.accel[1] = sy;
        sample.accel[2] = sz;
      } else {
        sample.gyro[0] = sx;
        sample.gyro[1] = sy;
        sample.gyro[2] = sz;
      }
    }
  }
  send_report(state);
}

void SwitchJoypad::set_gyro(float x, float y, float z) {
  apply_switch_imu(*this->_state, /* is_accel */ false, x, y, z);
}

void SwitchJoypad::set_accel(float x, float y, float z) {
  apply_switch_imu(*this->_state, /* is_accel */ true, x, y, z);
}

void SwitchJoypad::set_on_rumble(const std::function<void(int low_freq, int high_freq)> &callback) {
  this->_state->on_rumble = callback;
}

namespace {

constexpr auto SWITCH_VENDOR_ID = "057e";
constexpr auto SWITCH_PRODUCT_ID = "2009";

std::string uppercase_ascii(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::toupper(ch));
  });
  return value;
}

std::map<std::string, std::string> read_uevent_properties(const std::filesystem::path &uevent_path) {
  std::map<std::string, std::string> props;
  std::ifstream uevent_file(uevent_path);
  std::string line;
  while (std::getline(uevent_file, line)) {
    if (auto split = line.find('='); split != std::string::npos) {
      auto value = line.substr(split + 1);
      if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        value = value.substr(1, value.size() - 2);
      }
      props.emplace(line.substr(0, split), value);
    }
  }
  return props;
}

std::map<std::string, std::string> switch_input_identity(const std::filesystem::path &sys_entry,
                                                         const std::string &fallback_uniq) {
  auto props = read_uevent_properties(sys_entry / "uevent");
  const auto uniq = uppercase_ascii(props.count("UNIQ") ? props.at("UNIQ") : fallback_uniq);
  return {
      {"UNIQ", uniq},
      {"HID_UNIQ", uniq},
      {"PHYS", props.count("PHYS") ? props.at("PHYS") : "bluetooth"},
      {"MODALIAS", props.count("MODALIAS") ? props.at("MODALIAS") : "hid:b0005g0001v0000057Ep00002009"},
  };
}

std::map<std::string, std::string> switch_hid_identity(const std::filesystem::path &base_path,
                                                       const std::string &fallback_uniq) {
  auto props = read_uevent_properties(base_path / "uevent");
  const auto uniq = uppercase_ascii(props.count("HID_UNIQ") ? props.at("HID_UNIQ") : fallback_uniq);
  return {
      {"HID_UNIQ", uniq},
      {"HID_PHYS", props.count("HID_PHYS") ? props.at("HID_PHYS") : "bluetooth"},
      {"MODALIAS", props.count("MODALIAS") ? props.at("MODALIAS") : "hid:b0005g0001v0000057Ep00002009"},
  };
}

// Stamp the Switch Pro Bluetooth HID identity onto a udev event map.
void append_switch_identity(std::map<std::string, std::string> &event,
                            const std::map<std::string, std::string> &identity) {
  for (const auto &[key, value] : identity) {
    if (!value.empty()) {
      event[key] = value;
    }
  }
  event["ID_BUS"] = "bluetooth";
  event["HID_NAME"] = "Pro Controller";
  event["HID_ID"] = "0005:0000057E:00002009";
  event["ID_VENDOR_ID"] = SWITCH_VENDOR_ID;
  event["ID_MODEL_ID"] = SWITCH_PRODUCT_ID;
}

// Same identity, as hwdb "E:KEY=VALUE" rows appended to an entry.
void append_switch_identity(std::vector<std::string> &entry, const std::map<std::string, std::string> &identity) {
  std::vector<std::string> rows = {
      "E:ID_BUS=bluetooth",
      "E:HID_NAME=Pro Controller",
      "E:HID_ID=0005:0000057E:00002009",
      "E:ID_VENDOR_ID=057e",
      "E:ID_MODEL_ID=2009",
      "E:ID_VENDOR=Nintendo",
      "E:ID_MODEL=Pro_Controller",
      "E:ID_SERIAL=Nintendo_Pro_Controller",
  };
  for (const auto &[key, value] : identity) {
    if (!value.empty()) {
      rows.push_back("E:" + key + "=" + value);
    }
  }
  entry.insert(entry.end(), rows.begin(), rows.end());
}

} // namespace

std::vector<Joypad::UdevEvent> SwitchJoypad::get_udev_events() const {
  std::vector<Joypad::UdevEvent> events;
  const auto uniq = this->get_mac_address();

  auto sys_nodes = this->get_sys_nodes();
  for (const auto &sys_entry : sys_nodes) {
    for (const auto &sys_node : std::filesystem::directory_iterator{sys_entry}) {
      if (!sys_node.is_directory() || sys_node.path().filename().string().rfind("event", 0) != 0) {
        continue;
      }

      auto sys_path = sys_node.path().string();
      sys_path.erase(0, 4); // strip leading /sys
      auto dev_path = ("/dev/input/" / sys_node.path().filename()).string();
      auto event = gen_udev_base_event(dev_path, sys_path);
      auto identity = switch_input_identity(sys_entry, uniq);

      std::ifstream name_file(std::filesystem::path(sys_entry) / "name");
      std::string name;
      std::getline(name_file, name);

      if (name.find("Motion") != std::string::npos || name.find("IMU") != std::string::npos) {
        event["ID_INPUT_ACCELEROMETER"] = "1";
        event["ID_INPUT_WIDTH_MM"] = "8";
        event["ID_INPUT_HEIGHT_MM"] = "8";
        event["IIO_SENSOR_PROXY_TYPE"] = "input-accel";
        event["SYSTEMD_WANTS"] = "iio-sensor-proxy.service";
        event["UNIQ"] = uniq;
      } else {
        event["ID_INPUT_JOYSTICK"] = "1";
        event[".INPUT_CLASS"] = "joystick";
      }
      append_switch_identity(event, identity);
      events.emplace_back(event);
    }
  }

  if (!sys_nodes.empty()) {
    auto base_path = std::filesystem::path(sys_nodes[0]).parent_path().parent_path();
    if (std::filesystem::exists(base_path / "hidraw")) {
      for (const auto &hidraw_entry : std::filesystem::directory_iterator{base_path / "hidraw"}) {
        auto dev_path = "/dev/" + hidraw_entry.path().filename().string();
        auto sys_path = hidraw_entry.path().string();
        sys_path.erase(0, 4); // strip leading /sys
        auto event = gen_udev_base_event(dev_path, sys_path);
        event["SUBSYSTEM"] = "hidraw";
        append_switch_identity(event, switch_hid_identity(base_path, uniq));
        events.emplace_back(event);
      }
    }
  }

  return events;
}

std::vector<Joypad::UdevHwDbEntry> SwitchJoypad::get_udev_hw_db_entries() const {
  std::vector<Joypad::UdevHwDbEntry> result;
  const auto uniq = this->get_mac_address();

  for (const auto &sys_entry : this->get_sys_nodes()) {
    for (const auto &sys_node : std::filesystem::directory_iterator{sys_entry}) {
      if (!sys_node.is_directory() || sys_node.path().filename().string().rfind("event", 0) != 0) {
        continue;
      }

      auto dev_path = ("/dev/input/" / sys_node.path().filename()).string();
      Joypad::UdevHwDbEntry entry;
      entry.first = gen_udev_hw_db_filename(dev_path);

      std::ifstream name_file(std::filesystem::path(sys_entry) / "name");
      std::string name;
      std::getline(name_file, name);
      auto identity = switch_input_identity(sys_entry, uniq);

      if (name.find("Motion") != std::string::npos || name.find("IMU") != std::string::npos) {
        entry.second =
            {"E:ID_INPUT=1", "E:ID_INPUT_ACCELEROMETER=1", "G:seat", "G:uaccess", "Q:seat", "Q:uaccess", "V:1"};
      } else {
        entry.second = {"E:ID_INPUT=1", "E:ID_INPUT_JOYSTICK=1", "G:seat", "G:uaccess", "Q:seat", "Q:uaccess", "V:1"};
      }
      append_switch_identity(entry.second, identity);
      result.emplace_back(entry);
    }
  }

  auto sys_nodes = this->get_sys_nodes();
  if (!sys_nodes.empty()) {
    auto base_path = std::filesystem::path(sys_nodes[0]).parent_path().parent_path();
    if (std::filesystem::exists(base_path / "hidraw")) {
      for (const auto &hidraw_entry : std::filesystem::directory_iterator{base_path / "hidraw"}) {
        auto dev_path = "/dev/" + hidraw_entry.path().filename().string();
        std::vector<std::string> rows = {"E:SUBSYSTEM=hidraw", "G:seat", "G:uaccess", "Q:seat", "Q:uaccess", "V:1"};
        append_switch_identity(rows, switch_hid_identity(base_path, uniq));
        result.push_back({gen_udev_hw_db_filename(dev_path), rows});
      }
    }
  }

  return result;
}

} // namespace inputtino
