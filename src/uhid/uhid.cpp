#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <uhid/uhid.hpp>

namespace uhid {

std::vector<std::string> find_uhid_sys_nodes(uint16_t vendor_id, const inputtino::Mac &mac) {
  const std::string base_path = "/sys/devices/virtual/misc/uhid";
  std::vector<std::string> nodes;
  if (!std::filesystem::exists(base_path)) {
    return nodes;
  }

  std::ostringstream target_id;
  target_id << std::uppercase << std::hex << std::setfill('0') << std::setw(4)
            << static_cast<unsigned int>(vendor_id);

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
      if (mac.matches(uniq_value)) {
        nodes.push_back(dev_entry.path().string());
      }
    }
  }
  return nodes;
}

} // namespace uhid
