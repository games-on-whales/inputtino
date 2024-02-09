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
  static Result<std::shared_ptr<Mouse>> create();

  Mouse(const Mouse &j) : _state(j._state) {}
  Mouse(Mouse &&j) : _state(std::move(j._state)) {}
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
  static Result<std::shared_ptr<Trackpad>> create();
  Trackpad(const Trackpad &j) : _state(j._state) {}
  Trackpad(Trackpad &&j) : _state(std::move(j._state)) {}
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
  static Result<std::shared_ptr<TouchScreen>> create();
  TouchScreen(const TouchScreen &j) : _state(j._state) {}
  TouchScreen(TouchScreen &&j) : _state(std::move(j._state)) {}
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
  static Result<std::shared_ptr<PenTablet>> create();
  PenTablet(const PenTablet &j) : _state(j._state) {}
  PenTablet(PenTablet &&j) : _state(std::move(j._state)) {}

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
  static Result<std::shared_ptr<Keyboard>> create(std::chrono::milliseconds timeout_repress_key = 50ms);
  Keyboard(const Keyboard &j) : _state(j._state) {}
  Keyboard(Keyboard &&j) : _state(std::move(j._state)) {}

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
 * An abstraction on top of a virtual joypad
 * In order to support callbacks (ex: on_rumble()) this will create a new thread for listening for such events
 */
class Joypad : public VirtualDevice {
public:
  enum CONTROLLER_TYPE : uint8_t {
    UNKNOWN = 0x00,
    XBOX = 0x01,
    PS = 0x02,
    NINTENDO = 0x03
  };

  enum CONTROLLER_CAPABILITIES : uint8_t {
    ANALOG_TRIGGERS = 0x01,
    RUMBLE = 0x02,
    TRIGGER_RUMBLE = 0x04,
    TOUCHPAD = 0x08,
    ACCELEROMETER = 0x10,
    GYRO = 0x20,
    BATTERY = 0x40,
    RGB_LED = 0x80
  };

  static Result<std::shared_ptr<Joypad>> create(CONTROLLER_TYPE type, uint8_t capabilities);

  Joypad(const Joypad &j) : _state(std::move(j._state)) {}
  Joypad(Joypad &&j) : _state(std::move(j._state)) {}

  std::vector<std::string> get_nodes() const override;

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

  /**
   * Given the nature of joypads we (might) have to simultaneously press and release multiple buttons.
   * In order to implement this, you can pass a single short: button_flags which represent the currently pressed
   * buttons in the joypad.
   * This class will keep an internal state of the joypad and will automatically release buttons that are no
   * longer pressed.
   *
   * Example: previous state had `DPAD_UP` and `A` -> user release `A` -> new state only has `DPAD_UP`
   */
  void set_pressed_buttons(int newly_pressed);

  void set_triggers(int16_t left, int16_t right);

  enum STICK_POSITION {
    RS,
    LS
  };

  void set_stick(STICK_POSITION stick_type, short x, short y);

  void set_on_rumble(const std::function<void(int low_freq, int high_freq)> &callback);

  /**
   * If the joypad has been created with the TOUCHPAD capability this will return the associated trackpad
   */
  std::shared_ptr<Trackpad> get_trackpad() const;

  enum MOTION_TYPE : uint8_t {
    ACCELERATION = 0x01,
    GYROSCOPE = 0x02
  };

  void set_motion(MOTION_TYPE type, float x, float y, float z);

  enum BATTERY_STATE : uint8_t {
    NOT_KNOWN = 0x00,
    NOT_PRESENT = 0x01,
    DISCHARGING = 0x02,
    CHARGHING = 0x03,
    NOT_CHARGING = 0x04,
    FULL = 0x05
  };

  void set_battery(BATTERY_STATE state, int percentage);

  void set_on_led(const std::function<void(int r, int g, int b)> &callback);

protected:
  typedef struct JoypadState JoypadState;
  std::shared_ptr<JoypadState> _state;

private:
  Joypad();
};
} // namespace inputtino
