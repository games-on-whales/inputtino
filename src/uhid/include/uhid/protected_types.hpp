#pragma once
#include <functional>
#include <inputtino/input.hpp>
#include <optional>
#include <uhid/ps5.hpp>
#include <uhid/uhid.hpp>

namespace inputtino {
struct PS5JoypadState {
  std::shared_ptr<uhid::Device> dev;
  Mac mac;
  uint16_t vendor_id;

  uhid::dualsense_input_report current_state = {};
  uint8_t last_touch_id = 0;

  std::optional<std::function<void(int, int)>> on_rumble = std::nullopt;
  std::optional<std::function<void(int, int, int)>> on_led = std::nullopt;
  std::optional<std::function<void(const PS5Joypad::TriggerEffect &)>> on_trigger_effect = std::nullopt;
  uint32_t last_left_trigger_event = 0;
  uint32_t last_right_trigger_event = 0;

  bool stop_repeat_thread = false;
  bool is_bluetooth = true;
};
} // namespace inputtino
