#include <inputtino/mac.hpp>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <set>
#include <sstream>

namespace inputtino {

namespace {

// Prefix: AA:BB:CC:00:xx:xx — distinctive enough to avoid collisions with
// Docker (02:42:xx), VMs, or real hardware.
Mac from_id(uint16_t id) {
  return {
      {0xAA, 0xBB, 0xCC, 0x00, static_cast<unsigned char>((id >> 8) & 0xFF), static_cast<unsigned char>(id & 0xFF)}};
}

uint16_t to_id(const Mac &mac) {
  return (static_cast<uint16_t>(mac.bytes[4]) << 8) | static_cast<uint16_t>(mac.bytes[5]);
}

// Check whether a MAC is already in use by an existing UHID device.
bool is_active_on_system(const Mac &mac) {
  const std::string base = "/sys/devices/virtual/misc/uhid";
  if (!std::filesystem::exists(base)) {
    return false;
  }
  for (const auto &uhid_entry : std::filesystem::directory_iterator{base}) {
    if (!uhid_entry.is_directory())
      continue;
    auto input_path = uhid_entry.path() / "input";
    if (!std::filesystem::exists(input_path))
      continue;
    for (const auto &dev_entry : std::filesystem::directory_iterator{input_path}) {
      auto uniq_path = dev_entry.path() / "uniq";
      if (!std::filesystem::exists(uniq_path))
        continue;
      std::ifstream f{uniq_path};
      std::string uniq;
      std::getline(f, uniq);
      if (mac.matches(uniq)) {
        return true;
      }
    }
  }
  return false;
}

std::set<uint16_t> &pool() {
  static std::set<uint16_t> instance;
  return instance;
}

std::mutex &pool_mutex() {
  static std::mutex instance;
  return instance;
}

} // namespace

Mac Mac::generate() {
  std::lock_guard<std::mutex> lock(pool_mutex());
  auto &used = pool();
  uint16_t id = 1;
  while (used.count(id) || is_active_on_system(from_id(id))) {
    ++id;
  }
  used.insert(id);
  return from_id(id);
}

void Mac::release(const Mac &mac) {
  std::lock_guard<std::mutex> lock(pool_mutex());
  pool().erase(to_id(mac));
}

Mac Mac::parse(const std::string &str) {
  Mac mac;
  std::stringstream ss(str);
  for (int i = 0; i < 6; ++i) {
    unsigned int v = 0;
    ss >> std::hex >> v;
    mac.bytes[i] = static_cast<unsigned char>(v);
    if (i < 5)
      ss.ignore(1, ':');
  }
  return mac;
}

std::string Mac::to_string() const {
  std::ostringstream ss;
  for (int i = 0; i < 6; ++i) {
    if (i > 0)
      ss << ':';
    ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(bytes[i]);
  }
  return ss.str();
}

} // namespace inputtino
