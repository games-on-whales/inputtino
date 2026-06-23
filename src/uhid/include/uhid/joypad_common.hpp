#pragma once

// Shared plumbing for the uhid joypad backends (PS5 / Switch Pro). Both create a
// uhid device, run a report-pump thread, and guard the device with a mutex. Only
// the report layout (send_report) and
// the pump cadence differ, so the lifecycle lives here once.

#include <chrono>
#include <cstdio>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <uhid/uhid.hpp>
#include <vector>

namespace inputtino::uhid_joypad {

/**
 * State common to every uhid joypad. Concrete joypad states inherit this and add
 * their report-specific fields. `mtx` serialises access to `dev` between the
 * report-pump thread and the client-driven control thread (set_motion /
 * buttons).
 */
struct CommonState {
  std::shared_ptr<uhid::Device> dev;
  uhid::DeviceDefinition def = {};
  std::mutex mtx;
  bool stop_repeat_thread = false;
};

/**
 * Block until the kernel has exposed the device's sysfs input/hidraw nodes (or we
 * give up after max_attempts). Callers that read get_udev_events()/get_nodes()
 * straight after create() (e.g. plugging the pad into a container)
 * would otherwise see an empty device and it wouldn't enumerate.
 */
inline void wait_for_sys_nodes(const std::function<std::vector<std::string>()> &get_sys_nodes,
                               int max_attempts = 20,
                               std::chrono::milliseconds interval = std::chrono::milliseconds(50)) {
  for (int attempt = 0; attempt < max_attempts && get_sys_nodes().empty(); ++attempt) {
    std::this_thread::sleep_for(interval);
  }
}

/**
 * Start the report-pump thread: re-sends the HID input report every `interval` so
 * the device stays "live" for readers even when its state hasn't changed. The
 * thread holds a shared_ptr to the state so it stays valid across joypad moves,
 * and exits once state->stop_repeat_thread is set.
 */
template <typename State>
std::thread
start_report_pump(std::shared_ptr<State> state, std::function<void(State &)> send, std::chrono::milliseconds interval) {
  return std::thread([state, send = std::move(send), interval]() {
    while (!state->stop_repeat_thread) {
      send(*state);
      std::this_thread::sleep_for(interval);
    }
  });
}

} // namespace inputtino::uhid_joypad
