#pragma once

// Helpers for building the synthetic udev event + hwdb metadata returned by
// Joypad::get_udev_events() / get_udev_hw_db_entries(). A virtual device shows
// up as a bare /dev/input node with no udev database backing it; a consumer
// running apps inside a namespace replays these so libudev/SDL recognise the
// device as real hardware. Kept header-only (inline) and free of libevdev so
// both the uinput and uhid joypad backends can use them.

#include <chrono>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <utility>

namespace inputtino {

inline std::pair<unsigned int, unsigned int> get_major_minor(const std::string &devnode) {
  struct stat buf {};
  if (stat(devnode.c_str(), &buf) == -1) {
    std::cerr << "inputtino: unable to stat " << devnode << std::endl;
    return {};
  }
  if (!S_ISCHR(buf.st_mode)) {
    std::cerr << "inputtino: device " << devnode << " is not a character device" << std::endl;
    return {};
  }
  return {major(buf.st_rdev), minor(buf.st_rdev)};
}

// hwdb entries are keyed by a "c<major>:<minor>" filename (character device).
inline std::string gen_udev_hw_db_filename(const std::string &dev_node) {
  auto [dev_major, dev_minor] = get_major_minor(dev_node);
  return "c" + std::to_string(dev_major) + ":" + std::to_string(dev_minor);
}

// The properties every input udev event carries; callers add the device-class
// specific keys (ID_INPUT_JOYSTICK, ID_INPUT_ACCELEROMETER, ...) on top.
inline std::map<std::string, std::string>
gen_udev_base_event(const std::string &devnode, const std::string &syspath, const std::string &action = "add") {
  auto [dev_major, dev_minor] = get_major_minor(devnode);
  auto now = std::chrono::system_clock::now();
  auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
  return {
      {"ACTION", action},
      {"SEQNUM", "7"}, // We don't keep global state, let's hope it's not used
      {"USEC_INITIALIZED", std::to_string(timestamp)},
      {"SUBSYSTEM", "input"},
      {"ID_INPUT", "1"},
      {"ID_SERIAL", "noserial"},
      {"TAGS", ":seat:uaccess:"},
      {"CURRENT_TAGS", ":seat:uaccess:"},
      {"DEVNAME", devnode},
      {"DEVPATH", syspath},
      {"MAJOR", std::to_string(dev_major)},
      {"MINOR", std::to_string(dev_minor)},
  };
}

} // namespace inputtino
