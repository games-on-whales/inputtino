#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <inputtino/mac.hpp>
#include <inputtino/result.hpp>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace inputtino {

/**
 * Returns true when this host can create UHID virtual devices.
 *
 * UHID is the kernel interface that lets inputtino emit a "real"
 * HID device (PS5 DualSense, Switch Pro, ...) the host can read
 * via the usual hidraw / evdev path — including the rich
 * accelerometer/gyro/touchpad subdevices that PS5Joypad and
 * SwitchJoypad expose. It requires `/dev/uhid` (root, plus the
 * udev rule in `src/uhid/README.adoc`); on locked-down hosts it
 * doesn't exist and the fallback uinput implementations
 * (PS5JoypadUinput, SwitchJoypadUinput, XboxOneJoypad) should be used
 * instead.
 *
 * Probes by `open("/dev/uhid", O_RDWR)` once and caches the result
 * for the process lifetime — the device node doesn't appear or
 * disappear mid-run on any sane host.
 */
bool is_uhid_supported();

class VirtualDevice {
public:
  virtual std::vector<std::string> get_nodes() const = 0;

  /**
   * udev self-description.
   *
   * A device created via uinput/uhid shows up as a bare node under /dev/input
   * with no backing udev database. Consumers that run apps inside a mount/PID
   * namespace (e.g. a container) need to synthesise the udev events and hwdb
   * entries so libudev/SDL inside the namespace recognise the device as real,
   * fully-described hardware. Since this library created the device — and thus
   * knows its nodes, vendor/product ids and (for uhid) the HID identity — it is
   * the natural place to describe it.
   *
   * Both default to empty so devices/platforms that don't need this are
   * unaffected.
   */
  using UdevEvent = std::map<std::string, std::string>;
  using UdevHwDbEntry = std::pair<std::string /* filename */, std::vector<std::string> /* rows */>;
  virtual std::vector<UdevEvent> get_udev_events() const {
    return {};
  }
  virtual std::vector<UdevHwDbEntry> get_udev_hw_db_entries() const {
    return {};
  }

  virtual ~VirtualDevice() = default;
};

struct DeviceDefinition {
  std::string name;
  uint16_t vendor_id;
  uint16_t product_id;
  uint16_t version;

  std::string device_phys = "";
  std::string device_uniq = "";
};

/**
 * A virtual mouse device
 */
class Mouse : public VirtualDevice {
public:
  static Result<Mouse>
  create(const DeviceDefinition &device = {
             .name = "Wolf mouse virtual device", .vendor_id = 0xAB00, .product_id = 0xAB01, .version = 0xAB00});

  Mouse(Mouse &&j) noexcept : _state(nullptr) {
    std::swap(j._state, _state);
  }
  ~Mouse() override;
  std::vector<std::string> get_nodes() const override;

  void move(int delta_x, int delta_y);

  void move_abs(int x, int y, int screen_width, int screen_height);

  enum MOUSE_BUTTON {
    LEFT,
    MIDDLE,
    RIGHT,
    SIDE,
    EXTRA
  };

  void press(MOUSE_BUTTON button);

  void release(MOUSE_BUTTON button);

  /**
   *
   * A value that is a fraction of ±120 indicates a wheel movement less than
   * one logical click, a caller should either scroll by the respective
   * fraction of the normal scroll distance or accumulate that value until a
   * multiple of 120 is reached.
   *
   * The magic number 120 originates from the
   * <a href="http://download.microsoft.com/download/b/d/1/bd1f7ef4-7d72-419e-bc5c-9f79ad7bb66e/wheel.docx">
   * Windows Vista Mouse Wheel design document
   * </a>.
   *
   * Positive numbers will scroll down, negative numbers will scroll up
   *
   * @param high_res_distance The distance in high resolution
   */
  void vertical_scroll(int high_res_distance);

  /**
   *
   * A value that is a fraction of ±120 indicates a wheel movement less than
   * one logical click, a caller should either scroll by the respective
   * fraction of the normal scroll distance or accumulate that value until a
   * multiple of 120 is reached.
   *
   * The magic number 120 originates from the
   * <a href="http://download.microsoft.com/download/b/d/1/bd1f7ef4-7d72-419e-bc5c-9f79ad7bb66e/wheel.docx">
   * Windows Vista Mouse Wheel design document
   * </a>.
   *
   * Positive numbers will scroll right, negative numbers will scroll left
   *
   * @param high_res_distance The distance in high resolution
   */
  void horizontal_scroll(int high_res_distance);

protected:
  typedef struct MouseState MouseState;
  std::shared_ptr<MouseState> _state;

private:
  Mouse(); // use Mouse::create() instead
};

/**
 * A virtual trackpad
 *
 * implements a pure multi-touch touchpad as defined in libinput
 * https://wayland.freedesktop.org/libinput/doc/latest/touchpads.html
 */
class Trackpad : public VirtualDevice {
public:
  static Result<Trackpad>
  create(const DeviceDefinition &device = {
             .name = "Wolf (virtual) touchpad", .vendor_id = 0xAB00, .product_id = 0xAB02, .version = 0xAB00});
  Trackpad(Trackpad &&j) noexcept : _state(nullptr) {
    std::swap(j._state, _state);
  }
  ~Trackpad() override;
  std::vector<std::string> get_nodes() const override;

  /**
   * We expect (x,y) to be in the range [0.0, 1.0]; x and y values are normalised device coordinates
   * from the top-left corner (0.0, 0.0) to bottom-right corner (1.0, 1.0)
   *
   * @param finger_nr
   * @param pressure A value between 0 and 1
   * @param orientation A value between -90 and 90
   */
  void place_finger(int finger_nr, float x, float y, float pressure, int orientation);

  void release_finger(int finger_nr);

  void set_left_btn(bool pressed);

protected:
  typedef struct TrackpadState TrackpadState;
  std::shared_ptr<TrackpadState> _state;

private:
  Trackpad(); // use Trackpad::create() instead
};

/**
 * A virtual touchscreen
 */
class TouchScreen : public VirtualDevice {

public:
  static Result<TouchScreen>
  create(const DeviceDefinition &device = {
             .name = "Wolf (virtual) touchscreen", .vendor_id = 0xAB00, .product_id = 0xAB03, .version = 0xAB00});
  TouchScreen(TouchScreen &&j) noexcept : _state(nullptr) {
    std::swap(j._state, _state);
  }
  ~TouchScreen() override;
  std::vector<std::string> get_nodes() const override;

  /**
   * We expect (x,y) to be in the range [0.0, 1.0]; x and y values are normalised device coordinates
   * from the top-left corner (0.0, 0.0) to bottom-right corner (1.0, 1.0)
   *
   * @param finger_nr
   * @param pressure A value between 0 and 1
   */
  void place_finger(int finger_nr, float x, float y, float pressure, int orientation);

  void release_finger(int finger_nr);

protected:
  typedef struct TouchScreenState TouchScreenState;
  std::shared_ptr<TouchScreenState> _state;

private:
  TouchScreen();
};

/**
 * A virtual pen tablet
 *
 * implements a pen tablet as defined in libinput
 * https://wayland.freedesktop.org/libinput/doc/latest/tablet-support.html
 */
class PenTablet : public VirtualDevice {
public:
  static Result<PenTablet>
  create(const DeviceDefinition &device = {
             .name = "Wolf (virtual) pen tablet", .vendor_id = 0xAB00, .product_id = 0xAB04, .version = 0xAB00});
  PenTablet(PenTablet &&j) : _state(nullptr) {
    std::swap(j._state, _state);
  }
  ~PenTablet() override;
  std::vector<std::string> get_nodes() const override;

  enum TOOL_TYPE {
    PEN,
    ERASER,
    BRUSH,
    PENCIL,
    AIRBRUSH,
    TOUCH,
    SAME_AS_BEFORE /* Real devices don't need to report the tool type when it's still the same */
  };

  enum BTN_TYPE {
    PRIMARY,
    SECONDARY,
    TERTIARY
  };

  /**
   * x,y,pressure and distance should be normalized in the range [0.0, 1.0].
   * Passing a negative value will discard that value; this is used to report pressure instead of distance
   * (they should never be both positive).
   *
   * tilt_x and tilt_y are in the range [-90.0, 90.0] degrees.
   *
   * Refer to the libinput docs to better understand what each param means:
   * https://wayland.freedesktop.org/libinput/doc/latest/tablet-support.html#special-axes-on-tablet-tools
   */
  void place_tool(TOOL_TYPE tool_type, float x, float y, float pressure, float distance, float tilt_x, float tilt_y);

  void set_btn(BTN_TYPE btn, bool pressed);

protected:
  typedef struct PenTabletState PenTabletState;
  std::shared_ptr<PenTabletState> _state;

private:
  PenTablet();
};

/**
 * A virtual keyboard device
 *
 * Key codes are Win32 Virtual Key (VK) codes
 * Users of this class can expect that if a key is pressed, it'll be re-pressed every
 * time_repress_key until it's released.
 */
class Keyboard : public VirtualDevice {
public:
  static Result<Keyboard> create(const DeviceDefinition &device = {.name = "Wolf (virtual) keyboard",
                                                                   .vendor_id = 0xAB00,
                                                                   .product_id = 0xAB05,
                                                                   .version = 0xAB00},
                                 int millis_repress_key = 50);
  Keyboard(Keyboard &&j) noexcept : _state(nullptr) {
    std::swap(j._state, _state);
  }
  ~Keyboard() override;
  std::vector<std::string> get_nodes() const override;

  void press(short key_code);

  void release(short key_code);

protected:
  typedef struct KeyboardState KeyboardState;
  std::shared_ptr<KeyboardState> _state;

private:
  Keyboard();
};

/**
 * Base class for all joypads, they at the very least have to implement buttons and triggers
 */
class Joypad : public VirtualDevice {
public:
  /**
   * Which kind of pad create() should build; the concrete uhid/uinput backend is
   * then chosen at runtime.
   */
  enum class TYPE {
    XBOX,
    PS,
    NINTENDO
  };

  enum CONTROLLER_BTN : unsigned int {
    DPAD_UP = 0x0001,
    DPAD_DOWN = 0x0002,
    DPAD_LEFT = 0x0004,
    DPAD_RIGHT = 0x0008,

    START = 0x0010,
    BACK = 0x0020,
    HOME = 0x0400,

    LEFT_STICK = 0x0040,
    RIGHT_STICK = 0x0080,
    LEFT_BUTTON = 0x0100,
    RIGHT_BUTTON = 0x0200,

    SPECIAL_FLAG = 0x0400,
    PADDLE1_FLAG = 0x010000,
    PADDLE2_FLAG = 0x020000,
    PADDLE3_FLAG = 0x040000,
    PADDLE4_FLAG = 0x080000,
    TOUCHPAD_FLAG = 0x100000, // Touchpad buttons on Sony controllers
    MISC_FLAG = 0x200000,     // Share/Mic/Capture/Mute buttons on various controllers

    A = 0x1000,
    B = 0x2000,
    X = 0x4000,
    Y = 0x8000
  };

  enum STICK_POSITION {
    RS,
    LS
  };

  /**
   * Runtime joypad factory.
   *
   * Picks the backend at runtime: when `prefer_uhid` is true and the host can
   * create uhid devices, returns the rich uhid pad (`PS5Joypad` / `SwitchJoypad`);
   * otherwise the basic uinput pad (`PS5JoypadUinput` / `SwitchJoypadUinput`).
   * `XBOX` is uinput-only. The pad is owned through this `Joypad` base, so any
   * capability is reachable via the base interface — unsupported ones are safe
   * no-ops; gate motion on `supports_motion()`.
   *
   * `prefer_uhid` defaults to `is_uhid_supported()`, so the common case ("rich
   * when possible, basic otherwise") needs no wiring. If the library was built
   * without the uhid sources, the uinput pad is always returned regardless.
   */
  static Result<std::unique_ptr<Joypad>> create(TYPE kind, bool prefer_uhid = is_uhid_supported());
  static Result<std::unique_ptr<Joypad>>
  create(TYPE kind, const DeviceDefinition &device, bool prefer_uhid = is_uhid_supported());

  /**
   * The default device identity (name / vendor / product / version) for a
   * TYPE — what create(kind, prefer_uhid) uses. Exposed so a consumer can
   * start from it and tweak a field (e.g. device_uniq) without hard-coding the
   * vendor/product ids itself.
   */
  static DeviceDefinition default_definition(TYPE kind);

  /**
   * Given the nature of joypads we (might) have to simultaneously press and release multiple buttons.
   * In order to implement this, you can pass a single short: button_flags which represent the currently pressed
   * buttons in the joypad.
   * This class will keep an internal state of the joypad and will automatically release buttons that are no
   * longer pressed.
   *
   * Example: previous state had `DPAD_UP` and `A` -> user release `A` -> new state only has `DPAD_UP`
   */
  virtual void set_pressed_buttons(unsigned int newly_pressed) = 0;

  virtual void set_triggers(int16_t left, int16_t right) = 0;

  virtual void set_stick(STICK_POSITION stick_type, short x, short y) = 0;

  // ---- Optional rich capabilities ----
  //
  // These default to no-ops / "unsupported" so a generic Joypad (e.g. the
  // std::unique_ptr<Joypad> returned by Joypad::create()) is fully usable for
  // every backend without the caller knowing the concrete type. Backends that
  // implement a feature override the relevant method; the basic uinput pads
  // inherit the no-ops. supports_motion() is the one capability a caller
  // typically branches on (to avoid asking a client to stream sensor data a pad
  // can't consume) — the setters themselves are always safe to call.

  enum BATTERY_STATE : uint8_t {
    BATTERY_DISCHARGING = 0x0,
    BATTERY_CHARGHING = 0x1,
    BATTERY_FULL = 0x2,
    VOLTAGE_OR_TEMPERATURE_OUT_OF_RANGE = 0xA,
    TEMPERATURE_ERROR = 0xB,
    CHARGHING_ERROR = 0xF
  };

  /**
   * Opaque adaptive-trigger blob sent to the controller. Some reverse-engineered
   * documentation: https://gist.github.com/Nielk1/6d54cc2c00d2201ccb8c2720ad7538db
   */
  struct TriggerEffect {
    /**
     * 0x04 - Right trigger
     * 0x08 - Left trigger
     */
    uint8_t event_flags;
    uint8_t type_left;
    uint8_t type_right;
    std::array<uint8_t, 10> left = {};
    std::array<uint8_t, 10> right = {};
  };

  static constexpr int touchpad_width = 1920;
  static constexpr int touchpad_height = 1080;

  /** Whether this pad forwards motion (accelerometer/gyroscope) to the host. */
  virtual bool supports_motion() const {
    return false;
  }

  virtual void set_on_rumble(const std::function<void(int low_freq, int high_freq)> & /* callback */) {}

  /**
   * Forward a gyroscope sample. Units are deg/s (the SDL / Moonlight
   * convention); x/y/z follow SDL's sensor axis convention. No-op on pads
   * without motion — gate on supports_motion().
   */
  virtual void set_gyro(float /* x */, float /* y */, float /* z */) {}

  /**
   * Forward an accelerometer sample. Units are m/s^2, inclusive of gravity;
   * x/y/z follow SDL's sensor axis convention. No-op on pads without motion —
   * gate on supports_motion().
   */
  virtual void set_accel(float /* x */, float /* y */, float /* z */) {}
  virtual void place_finger(int /* finger_nr */, uint16_t /* x */, uint16_t /* y */) {}
  virtual void release_finger(int /* finger_nr */) {}
  virtual void set_battery(BATTERY_STATE /* state */, int /* percentage */) {}
  virtual void set_on_led(const std::function<void(int r, int g, int b)> & /* callback */) {}
  virtual void set_on_trigger_effect(const std::function<void(const TriggerEffect &)> & /* callback */) {}
};

class XboxOneJoypad : public Joypad {
public:
  static Result<XboxOneJoypad>
  create(const DeviceDefinition &device = {
             .name = "Wolf X-Box One (virtual) pad",
             // https://github.com/torvalds/linux/blob/master/drivers/input/joystick/xpad.c#L147
             .vendor_id = 0x045E,
             .product_id = 0x02EA,
             .version = 0x0408});
  XboxOneJoypad(XboxOneJoypad &&j) noexcept : _state(nullptr) {
    std::swap(j._state, _state);
  }
  ~XboxOneJoypad() override;

  std::vector<std::string> get_nodes() const override;
  std::vector<UdevEvent> get_udev_events() const override;
  std::vector<UdevHwDbEntry> get_udev_hw_db_entries() const override;

  void set_pressed_buttons(unsigned int newly_pressed) override;
  void set_triggers(int16_t left, int16_t right) override;
  void set_stick(STICK_POSITION stick_type, short x, short y) override;
  void set_on_rumble(const std::function<void(int low_freq, int high_freq)> &callback);

protected:
  typedef struct XboxOneJoypadState XboxOneJoypadState;
  std::shared_ptr<XboxOneJoypadState> _state;

private:
  XboxOneJoypad();
};

class SwitchJoypad : public Joypad {
public:
  static Result<SwitchJoypad> create(const DeviceDefinition &device = {
                                         .name = "Wolf Nintendo (virtual) pad",
                                         // https://github.com/torvalds/linux/blob/master/drivers/hid/hid-ids.h#L981
                                         .vendor_id = 0x057e,
                                         .product_id = 0x2009,
                                         .version = 0x8111});
  SwitchJoypad(SwitchJoypad &&j) noexcept : _state(nullptr) {
    std::swap(j._state, _state);
    std::swap(j._send_input_thread, _send_input_thread);
  }
  ~SwitchJoypad() override;

  std::vector<std::string> get_nodes() const override;
  std::vector<UdevEvent> get_udev_events() const override;
  std::vector<UdevHwDbEntry> get_udev_hw_db_entries() const override;

  std::string get_mac_address() const;

  std::vector<std::string> get_sys_nodes() const;

  void set_pressed_buttons(unsigned int newly_pressed) override;
  void set_triggers(int16_t left, int16_t right) override;
  void set_stick(STICK_POSITION stick_type, short x, short y) override;
  bool supports_motion() const override {
    return true;
  }
  /**
   * Forward a gyroscope sample, in deg/s (SDL / Moonlight convention). Unlike
   * PS5's rad/s report no unit fix is needed — only the SDL->hid-nintendo frame
   * remap. Axes follow the SDL convention.
   */
  void set_gyro(float x, float y, float z) override;

  /**
   * Forward an accelerometer sample, in m/s^2 (inclusive of gravity); SDL axis
   * convention, with the SDL->hid-nintendo frame remap.
   */
  void set_accel(float x, float y, float z) override;
  void set_on_rumble(const std::function<void(int low_freq, int high_freq)> &callback) override;

protected:
  typedef struct SwitchJoypadState SwitchJoypadState;
  std::shared_ptr<SwitchJoypadState> _state;

private:
  std::thread _send_input_thread;

  SwitchJoypad(const Mac &mac);
};

class PS5Joypad : public Joypad {
public:
  static Result<PS5Joypad>
  create(const DeviceDefinition &device = {
             .name = "Wolf DualSense (virtual) pad", .vendor_id = 0x054C, .product_id = 0x0CE6, .version = 0x8111});
  PS5Joypad(PS5Joypad &&j) noexcept : _state(nullptr) {
    std::swap(j._state, _state);
    // The report pump is joinable (not detached) so the destructor can wait for it; it must
    // travel with the state it writes to, otherwise the moved-from object would terminate() on a joinable thread.
    std::swap(j._send_input_thread, _send_input_thread);
  }
  ~PS5Joypad() override;

  std::vector<std::string> get_nodes() const override;
  std::vector<UdevEvent> get_udev_events() const override;
  std::vector<UdevHwDbEntry> get_udev_hw_db_entries() const override;

  std::string get_mac_address() const;

  std::vector<std::string> get_sys_nodes() const;

  void set_pressed_buttons(unsigned int newly_pressed) override;
  void set_triggers(int16_t left, int16_t right) override;
  void set_stick(STICK_POSITION stick_type, short x, short y) override;
  void set_on_rumble(const std::function<void(int low_freq, int high_freq)> &callback) override;

  bool supports_motion() const override {
    return true;
  }

  void place_finger(int finger_nr, uint16_t x, uint16_t y) override;
  void release_finger(int finger_nr) override;

  enum MOTION_TYPE : uint8_t {
    ACCELERATION = 0x01, // sample in m/s^2 (inclusive of gravity)
    GYROSCOPE = 0x02     // sample in rad/s (legacy scaling; set_gyro() takes deg/s)
  };

  /**
   * @deprecated Legacy DualSense-only motion API, kept for source backwards-
   * compatibility (e.g. Sunshine, which pre-converts gyro to rad/s and calls
   * this). Prefer the generic set_gyro() (deg/s) / set_accel() (m/s^2).
   *
   * GYROSCOPE expects rad/s, ACCELERATION m/s^2 (inclusive of gravity). This is
   * now a thin shim: it forwards to set_accel() as-is and to set_gyro() after
   * converting gyro rad/s -> deg/s, so there is a single motion code path.
   *
   * The x/y/z axis assignments follow SDL's convention documented here:
   * https://github.com/libsdl-org/SDL/blob/96720f335002bef62115e39327940df454d78f6c/include/SDL3/SDL_sensor.h#L80-L124
   */
  void set_motion(MOTION_TYPE type, float x, float y, float z);

  /**
   * Forward a gyroscope sample, in deg/s (SDL / Moonlight convention). Converts
   * deg->rad internally for the rad/s-calibrated DualSense HID report.
   */
  void set_gyro(float x, float y, float z) override;

  /**
   * Forward an accelerometer sample, in m/s^2 (inclusive of gravity).
   */
  void set_accel(float x, float y, float z) override;

  void set_battery(BATTERY_STATE state, int percentage) override;

  void set_on_led(const std::function<void(int r, int g, int b)> &callback) override;

  void set_on_trigger_effect(const std::function<void(const TriggerEffect &)> &callback) override;

protected:
  typedef struct PS5JoypadState PS5JoypadState;
  std::shared_ptr<PS5JoypadState> _state;

private:
  std::thread _send_input_thread;

  PS5Joypad(uint16_t vendor_id, const Mac &mac);
};

/**
 * Basic uinput DualSense pad: buttons, sticks, triggers and rumble only — no
 * touchpad / motion / battery / LED / adaptive triggers (those inherit the
 * Joypad base no-ops). Universally available since it needs only uinput, so it
 * is the fallback Joypad::create() picks when /dev/uhid isn't accessible.
 */
class PS5JoypadUinput : public Joypad {
public:
  static Result<PS5JoypadUinput>
  create(const DeviceDefinition &device = {
             .name = "Wolf DualSense (virtual) pad", .vendor_id = 0x054C, .product_id = 0x0CE6, .version = 0x8111});
  PS5JoypadUinput(PS5JoypadUinput &&j) noexcept : _state(nullptr) {
    std::swap(j._state, _state);
  }
  ~PS5JoypadUinput() override;

  std::vector<std::string> get_nodes() const override;
  std::vector<UdevEvent> get_udev_events() const override;
  std::vector<UdevHwDbEntry> get_udev_hw_db_entries() const override;

  void set_pressed_buttons(unsigned int newly_pressed) override;
  void set_triggers(int16_t left, int16_t right) override;
  void set_stick(STICK_POSITION stick_type, short x, short y) override;
  void set_on_rumble(const std::function<void(int low_freq, int high_freq)> &callback) override;

protected:
  typedef struct PS5JoypadUinputState PS5JoypadUinputState;
  std::shared_ptr<PS5JoypadUinputState> _state;

private:
  PS5JoypadUinput();
};

/**
 * Basic uinput Nintendo pad: buttons (positionally remapped to the Xbox-style
 * Joypad API, matching SwitchJoypad), sticks and rumble. No motion (inherits
 * the base no-op + supports_motion() == false). uinput-only fallback.
 */
class SwitchJoypadUinput : public Joypad {
public:
  static Result<SwitchJoypadUinput>
  create(const DeviceDefinition &device = {
             .name = "Wolf Nintendo (virtual) pad", .vendor_id = 0x057e, .product_id = 0x2009, .version = 0x8111});
  SwitchJoypadUinput(SwitchJoypadUinput &&j) noexcept : _state(nullptr) {
    std::swap(j._state, _state);
  }
  ~SwitchJoypadUinput() override;

  std::vector<std::string> get_nodes() const override;
  std::vector<UdevEvent> get_udev_events() const override;
  std::vector<UdevHwDbEntry> get_udev_hw_db_entries() const override;

  void set_pressed_buttons(unsigned int newly_pressed) override;
  void set_triggers(int16_t left, int16_t right) override;
  void set_stick(STICK_POSITION stick_type, short x, short y) override;
  void set_on_rumble(const std::function<void(int low_freq, int high_freq)> &callback) override;

protected:
  typedef struct SwitchJoypadUinputState SwitchJoypadUinputState;
  std::shared_ptr<SwitchJoypadUinputState> _state;

private:
  SwitchJoypadUinput();
};

} // namespace inputtino
