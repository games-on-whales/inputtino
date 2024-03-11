#pragma once
#include <functional>
#include <optional>
#include <uhid/ps5.hpp>
#include <uhid/uhid.hpp>

namespace inputtino {
struct PS5JoypadState {
  std::shared_ptr<uhid::Device> dev;

  uhid::dualsense_input_report_usb current_state;

  std::optional<std::function<void(int, int)>> on_rumble = std::nullopt;
  std::optional<std::function<void(int, int, int)>> on_led = std::nullopt;
};
} // namespace inputtino