#include "inputtino/input.hpp"
#include <cmath>
#include <inputtino/protected_types.hpp>
#include <string.h>

namespace inputtino {

std::vector<std::string> Mouse::get_nodes() const {
  std::vector<std::string> nodes;

  if (auto mouse = _state->mouse_rel.get()) {
    nodes.emplace_back(libevdev_uinput_get_devnode(mouse));
  }

  if (auto mouse = _state->mouse_abs.get()) {
    nodes.emplace_back(libevdev_uinput_get_devnode(mouse));
  }

  return nodes;
}

constexpr int ABS_MAX_WIDTH = 19200;
constexpr int ABS_MAX_HEIGHT = 12000;

static Result<libevdev_uinput_ptr> create_mouse() {
  libevdev *dev = libevdev_new();
  libevdev_uinput *uidev;

  libevdev_set_name(dev, "Wolf mouse virtual device");
  libevdev_set_id_vendor(dev, 0xAB00);
  libevdev_set_id_product(dev, 0xAB01);
  libevdev_set_id_version(dev, 0xAB00);
  libevdev_set_id_bustype(dev, BUS_USB);

  libevdev_enable_event_type(dev, EV_KEY);
  libevdev_enable_event_code(dev, EV_KEY, BTN_LEFT, nullptr);
  libevdev_enable_event_code(dev, EV_KEY, BTN_RIGHT, nullptr);
  libevdev_enable_event_code(dev, EV_KEY, BTN_MIDDLE, nullptr);
  libevdev_enable_event_code(dev, EV_KEY, BTN_SIDE, nullptr);
  libevdev_enable_event_code(dev, EV_KEY, BTN_EXTRA, nullptr);
  libevdev_enable_event_code(dev, EV_KEY, BTN_FORWARD, nullptr);
  libevdev_enable_event_code(dev, EV_KEY, BTN_BACK, nullptr);
  libevdev_enable_event_code(dev, EV_KEY, BTN_TASK, nullptr);

  libevdev_enable_event_type(dev, EV_REL);
  libevdev_enable_event_code(dev, EV_REL, REL_X, nullptr);
  libevdev_enable_event_code(dev, EV_REL, REL_Y, nullptr);

  libevdev_enable_event_code(dev, EV_REL, REL_WHEEL, nullptr);
  libevdev_enable_event_code(dev, EV_REL, REL_WHEEL_HI_RES, nullptr);
  libevdev_enable_event_code(dev, EV_REL, REL_HWHEEL, nullptr);
  libevdev_enable_event_code(dev, EV_REL, REL_HWHEEL_HI_RES, nullptr);

  libevdev_enable_event_type(dev, EV_MSC);
  libevdev_enable_event_code(dev, EV_MSC, MSC_SCAN, nullptr);

  auto err = libevdev_uinput_create_from_device(dev, LIBEVDEV_UINPUT_OPEN_MANAGED, &uidev);
  libevdev_free(dev);
  if (err != 0) {
    return Error(strerror(-err));
  }

  return libevdev_uinput_ptr{uidev, ::libevdev_uinput_destroy};
}

static Result<libevdev_uinput_ptr> create_mouse_abs() {
  libevdev *dev = libevdev_new();
  libevdev_uinput *uidev;

  libevdev_set_name(dev, "Wolf mouse (abs) virtual device");
  libevdev_set_id_vendor(dev, 0xAB00);
  libevdev_set_id_product(dev, 0xAB02);
  libevdev_set_id_version(dev, 0xAB00);
  libevdev_set_id_bustype(dev, BUS_USB);

  libevdev_enable_property(dev, INPUT_PROP_DIRECT);
  libevdev_enable_event_type(dev, EV_KEY);
  libevdev_enable_event_code(dev, EV_KEY, BTN_LEFT, nullptr);

  struct input_absinfo absinfo {
    .value = 0, .minimum = 0, .maximum = 0, .fuzz = 1, .flat = 0, .resolution = 28
  };
  libevdev_enable_event_type(dev, EV_ABS);

  absinfo.maximum = ABS_MAX_WIDTH;
  libevdev_enable_event_code(dev, EV_ABS, ABS_X, &absinfo);
  absinfo.maximum = ABS_MAX_HEIGHT;
  libevdev_enable_event_code(dev, EV_ABS, ABS_Y, &absinfo);

  auto err = libevdev_uinput_create_from_device(dev, LIBEVDEV_UINPUT_OPEN_MANAGED, &uidev);
  libevdev_free(dev);
  if (err != 0) {
    return Error(strerror(-err));
  }

  return libevdev_uinput_ptr{uidev, ::libevdev_uinput_destroy};
}

Mouse::Mouse() : _state(std::make_shared<MouseState>()) {}

Mouse::~Mouse() {
  if (_state) {
    _state.reset();
  }
}

Result<Mouse> Mouse::create() {
  auto mouse = Mouse();

  auto mouse_rel_or_error = create_mouse();
  if (mouse_rel_or_error) {
    mouse._state->mouse_rel = std::move(*mouse_rel_or_error);
  } else {
    return Error(mouse_rel_or_error.getErrorMessage());
  }

  auto mouse_abs_or_error = create_mouse_abs();
  if (mouse_abs_or_error) {
    mouse._state->mouse_abs = std::move(*mouse_abs_or_error);
  } else {
    return Error(mouse_abs_or_error.getErrorMessage());
  }

  return std::move(mouse);
}

void Mouse::move(int delta_x, int delta_y) {
  if (auto mouse = _state->mouse_rel.get()) {
    libevdev_uinput_write_event(mouse, EV_REL, REL_X, delta_x);
    libevdev_uinput_write_event(mouse, EV_REL, REL_Y, delta_y);
    libevdev_uinput_write_event(mouse, EV_SYN, SYN_REPORT, 0);
  }
}

void Mouse::move_abs(int x, int y, int screen_width, int screen_height) {
  int scaled_x = (int)std::lround((ABS_MAX_WIDTH / screen_width) * x);
  int scaled_y = (int)std::lround((ABS_MAX_HEIGHT / screen_height) * y);

  if (auto mouse = _state->mouse_abs.get()) {
    libevdev_uinput_write_event(mouse, EV_ABS, ABS_X, scaled_x);
    libevdev_uinput_write_event(mouse, EV_ABS, ABS_Y, scaled_y);
    libevdev_uinput_write_event(mouse, EV_SYN, SYN_REPORT, 0);
  }
}

static std::pair<int, int> btn_to_uinput(Mouse::MOUSE_BUTTON button) {
  switch (button) {
  case Mouse::LEFT:
    return {BTN_LEFT, 90001};
  case Mouse::MIDDLE:
    return {BTN_MIDDLE, 90003};
  case Mouse::RIGHT:
    return {BTN_RIGHT, 90002};
  case Mouse::SIDE:
    return {BTN_SIDE, 90004};
  default:
    return {BTN_EXTRA, 90005};
  }
}

void Mouse::press(Mouse::MOUSE_BUTTON button) {
  if (auto mouse = _state->mouse_rel.get()) {
    auto [btn_type, scan_code] = btn_to_uinput(button);
    libevdev_uinput_write_event(mouse, EV_MSC, MSC_SCAN, scan_code);
    libevdev_uinput_write_event(mouse, EV_KEY, btn_type, 1);
    libevdev_uinput_write_event(mouse, EV_SYN, SYN_REPORT, 0);
  }
}

void Mouse::release(Mouse::MOUSE_BUTTON button) {
  if (auto mouse = _state->mouse_rel.get()) {
    auto [btn_type, scan_code] = btn_to_uinput(button);
    libevdev_uinput_write_event(mouse, EV_MSC, MSC_SCAN, scan_code);
    libevdev_uinput_write_event(mouse, EV_KEY, btn_type, 0);
    libevdev_uinput_write_event(mouse, EV_SYN, SYN_REPORT, 0);
  }
}

void Mouse::horizontal_scroll(int high_res_distance) {
  int distance = high_res_distance / 120;

  if (auto mouse = _state->mouse_rel.get()) {
    libevdev_uinput_write_event(mouse, EV_REL, REL_HWHEEL, distance);
    libevdev_uinput_write_event(mouse, EV_REL, REL_HWHEEL_HI_RES, high_res_distance);
    libevdev_uinput_write_event(mouse, EV_SYN, SYN_REPORT, 0);
  }
}

void Mouse::vertical_scroll(int high_res_distance) {
  int distance = high_res_distance / 120;

  if (auto mouse = _state->mouse_rel.get()) {
    libevdev_uinput_write_event(mouse, EV_REL, REL_WHEEL, distance);
    libevdev_uinput_write_event(mouse, EV_REL, REL_WHEEL_HI_RES, high_res_distance);
    libevdev_uinput_write_event(mouse, EV_SYN, SYN_REPORT, 0);
  }
}

} // namespace inputtino