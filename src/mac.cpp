#include <inputtino/mac.hpp>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <regex>
#include <sstream>

namespace inputtino {

namespace {

// Prefix: AA:BB:CC:00:xx:xx — distinctive enough to avoid collisions with
// Docker (02:42:xx), VMs, or real hardware.
Mac from_id(uint16_t id) {
  return {
      {0xAA, 0xBB, 0xCC, 0x00, static_cast<unsigned char>((id >> 8) & 0xFF), static_cast<unsigned char>(id & 0xFF)}};
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

uint16_t &next_id() {
  static uint16_t instance = 1;
  return instance;
}

std::mutex &next_id_mutex() {
  static std::mutex instance;
  return instance;
}

} // namespace

Result<Mac> Mac::generate() {
  std::lock_guard<std::mutex> lock(next_id_mutex());
  auto &id = next_id();
  auto start = id;
  while (is_active_on_system(from_id(id))) {
    ++id;
    if (id == start) {
      return Error("Exhausted the MAC address space");
    }
  }
  auto mac = from_id(id);
  ++id;
  return mac;
}

Result<Mac> Mac::parse(const std::string &str) {
  static const std::regex mac_format("^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$");
  if (!std::regex_match(str, mac_format)) {
    return Error("Invalid MAC address: " + str);
  }

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
