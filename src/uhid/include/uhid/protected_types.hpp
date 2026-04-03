#pragma once
#include <functional>
#include <inputtino/input.hpp>
#include <optional>
#include <uhid/joypad_common.hpp>
#include <uhid/ps5.hpp>
#include <uhid/switch.hpp>
#include <uhid/uhid.hpp>

namespace inputtino {
// dev / def / mtx / stop_repeat_thread live in uhid_joypad::CommonState, shared
// with PS5JoypadState below and driven by the helpers in uhid/joypad_common.hpp.
struct SwitchJoypadState : uhid_joypad::CommonState {
  /**
   * The device's Bluetooth address, reported to hid-nintendo and used as the
   * input nodes' uniq. Like the PS5 one it must be unique per virtual device.
   */
  std::array<unsigned char, 6> mac_address = {};
  uint16_t vendor_id;
  uint16_t product_id;
  uint8_t controller_type = uhid::JOYCON_CTLR_TYPE_PRO;

  uint8_t buttons[3] = {};
  uint16_t lx = uhid::SWITCH_AXIS_CENTER;
  uint16_t ly = uhid::SWITCH_AXIS_CENTER;
  uint16_t rx = uhid::SWITCH_AXIS_CENTER;
  uint16_t ry = uhid::SWITCH_AXIS_CENTER;
  uhid::switch_imu_sample imu[3] = {};

  uint8_t timer = 0;
  uint8_t report_mode = uhid::SWITCH_REPORT_MODE_STANDARD_FULL;
  bool imu_enabled = false;
  bool vibration_enabled = false;
  std::optional<std::function<void(int, int)>> on_rumble = std::nullopt;
};

struct PS5JoypadState : uhid_joypad::CommonState {
  /**
   * This will be the MAC address of the device
   *
   * IMPORTANT: this needs to be unique for each virtual device,
   * otherwise the kernel driver will return an error:
   * "Duplicate device found for MAC address XX:XX:XX:XX"
   *
   * We also use this information internally to unique match a device with the
   * /dev/input/devXX files; see get_nodes()
   */
  unsigned char mac_address[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
  uint16_t vendor_id;

  uhid::dualsense_input_report current_state = {};
  uint8_t last_touch_id = 0;

  std::optional<std::function<void(int, int)>> on_rumble = std::nullopt;
  std::optional<std::function<void(int, int, int)>> on_led = std::nullopt;
  std::optional<std::function<void(const PS5Joypad::TriggerEffect &)>> on_trigger_effect = std::nullopt;
  uint32_t last_left_trigger_event = 0;
  uint32_t last_right_trigger_event = 0;

  bool is_bluetooth = true;
};
} // namespace inputtino
