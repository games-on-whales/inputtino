#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <uhid/uhid.hpp>

namespace uhid {

std::optional<std::array<unsigned char, 6>> parse_mac(const std::string &str) {
  if (str.size() != 17) {
    return std::nullopt;
  }
  std::array<unsigned char, 6> mac = {};
  for (int i = 0; i < 6; ++i) {
    auto byte = str.substr(i * 3, 2);
    if (!std::isxdigit(static_cast<unsigned char>(byte[0])) || !std::isxdigit(static_cast<unsigned char>(byte[1])) ||
        (i < 5 && str[i * 3 + 2] != ':')) {
      return std::nullopt;
    }
    mac[i] = static_cast<unsigned char>(std::stoul(byte, nullptr, 16));
  }
  return mac;
}

std::string format_mac(const std::array<unsigned char, 6> &mac) {
  std::ostringstream stream;
  stream << std::hex << std::setfill('0');
  for (int i = 0; i < 6; ++i) {
    if (i > 0) {
      stream << ":";
    }
    stream << std::setw(2) << static_cast<unsigned int>(mac[i]);
  }
  return stream.str();
}

std::vector<std::string> find_uhid_sys_nodes(uint16_t vendor_id, const std::string &mac) {
  const std::string base_path = "/sys/devices/virtual/misc/uhid";
  std::vector<std::string> nodes;
  auto target_mac = parse_mac(mac);
  if (!target_mac || !std::filesystem::exists(base_path)) {
    return nodes;
  }

  std::ostringstream target_id;
  target_id << std::uppercase << std::hex << std::setfill('0') << std::setw(4) << static_cast<unsigned int>(vendor_id);

  for (const auto &uhid_entry : std::filesystem::directory_iterator{base_path}) {
    if (!uhid_entry.is_directory()) {
      continue;
    }
    if (uhid_entry.path().filename().string().find(target_id.str()) == std::string::npos) {
      continue;
    }
    auto input_path = uhid_entry.path() / "input";
    if (!std::filesystem::exists(input_path)) {
      continue;
    }
    for (const auto &dev_entry : std::filesystem::directory_iterator{input_path}) {
      if (!dev_entry.is_directory()) {
        continue;
      }
      auto uniq_path = dev_entry.path() / "uniq";
      if (!std::filesystem::exists(uniq_path)) {
        continue;
      }
      std::ifstream uniq_file{uniq_path};
      std::string uniq_value;
      std::getline(uniq_file, uniq_value);
      if (parse_mac(uniq_value) == target_mac) {
        nodes.push_back(dev_entry.path().string());
      }
    }
  }
  return nodes;
}

} // namespace uhid
