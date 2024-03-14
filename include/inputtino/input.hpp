#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <inputtino/result.hpp>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace inputtino {

using namespace std::chrono_literals;

class VirtualDevice {
public:
  virtual std::vector<std::string> get_nodes() const = 0;
  virtual ~VirtualDevice() = default;
};

/**
 * A virtual mouse device
 */
class Mouse : public VirtualDevice {
public:
  static Result<Mouse> create();

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
  static Result<Trackpad> create();
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
  static Result<TouchScreen> create();
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
  static Result<PenTablet> create();
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
  static Result<Keyboard> create(std::chrono::milliseconds timeout_repress_key = 50ms);
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
  enum CONTROLLER_BTN : int {
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
   * Given the nature of joypads we (might) have to simultaneously press and release multiple buttons.
   * In order to implement this, you can pass a single short: button_flags which represent the currently pressed
   * buttons in the joypad.
   * This class will keep an internal state of the joypad and will automatically release buttons that are no
   * longer pressed.
   *
   * Example: previous state had `DPAD_UP` and `A` -> user release `A` -> new state only has `DPAD_UP`
   */
  virtual void set_pressed_buttons(int newly_pressed) = 0;

  virtual void set_triggers(int16_t left, int16_t right) = 0;

  virtual void set_stick(STICK_POSITION stick_type, short x, short y) = 0;
};

class XboxOneJoypad : public Joypad {
public:
  static Result<XboxOneJoypad> create();
  XboxOneJoypad(XboxOneJoypad &&j) noexcept : _state(nullptr) {
    std::swap(j._state, _state);
  }
  ~XboxOneJoypad() override;

  std::vector<std::string> get_nodes() const override;

  void set_pressed_buttons(int newly_pressed) override;
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
  static Result<SwitchJoypad> create();
  SwitchJoypad(SwitchJoypad &&j) : _state(nullptr) {
    std::swap(j._state, _state);
  }
  ~SwitchJoypad() override;

  std::vector<std::string> get_nodes() const override;

  void set_pressed_buttons(int newly_pressed) override;
  void set_triggers(int16_t left, int16_t right) override;
  void set_stick(STICK_POSITION stick_type, short x, short y) override;
  void set_on_rumble(const std::function<void(int low_freq, int high_freq)> &callback);

protected:
  typedef struct XboxOneJoypadState SwitchJoypadState;
  std::shared_ptr<SwitchJoypadState> _state;

private:
  SwitchJoypad();
};

class PS5Joypad : public Joypad {
public:
  static Result<PS5Joypad> create();
  PS5Joypad(PS5Joypad &&j) noexcept : _state(nullptr) {
    std::swap(j._state, _state);
  }
  ~PS5Joypad() override;

  std::vector<std::string> get_nodes() const override;

  void set_pressed_buttons(int newly_pressed) override;
  void set_triggers(int16_t left, int16_t right) override;
  void set_stick(STICK_POSITION stick_type, short x, short y) override;
  void set_on_rumble(const std::function<void(int low_freq, int high_freq)> &callback);

  static constexpr int touchpad_width = 1920;
  static constexpr int touchpad_height = 1080;
  void place_finger(int finger_nr, uint16_t x, uint16_t y);
  void release_finger(int finger_nr);

  enum MOTION_TYPE : uint8_t {
    ACCELERATION = 0x01,
    GYROSCOPE = 0x02
  };

  /**
   * Acceleration should report data in m/s^2 (inclusive of gravitational acceleration).
   * Gyroscope should report data in deg/s.
   *
   * The x/y/z axis assignments follow SDL's convention documented here:
   * https://github.com/libsdl-org/SDL/blob/96720f335002bef62115e39327940df454d78f6c/include/SDL3/SDL_sensor.h#L80-L124
   */
  void set_motion(MOTION_TYPE type, float x, float y, float z);

  enum BATTERY_STATE : uint8_t {
    BATTERY_DISCHARGING = 0x0,
    BATTERY_CHARGHING = 0x1,
    BATTERY_FULL = 0x2,
    VOLTAGE_OR_TEMPERATURE_OUT_OF_RANGE = 0xA,
    TEMPERATURE_ERROR = 0xB,
    CHARGHING_ERROR = 0xF
  };

  void set_battery(BATTERY_STATE state, int percentage);

  void set_on_led(const std::function<void(int r, int g, int b)> &callback);

protected:
  typedef struct PS5JoypadState PS5JoypadState;
  std::shared_ptr<PS5JoypadState> _state;

private:
  PS5Joypad();
};
} // namespace inputtino
