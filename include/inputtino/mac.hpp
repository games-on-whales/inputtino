#pragma once

#include <array>
#include <functional>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

namespace inputtino {

struct Mac {
  std::array<unsigned char, 6> bytes = {};

  static Mac generate() {
    auto rand = std::bind(std::uniform_int_distribution<unsigned char>{0, 0xFF},
                          std::default_random_engine{std::random_device()()});
    return {{rand(), rand(), rand(), rand(), rand(), rand()}};
  }

  static Mac parse(const std::string &str) {
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

  std::string to_string() const {
    std::ostringstream ss;
    for (int i = 0; i < 6; ++i) {
      if (i > 0)
        ss << ':';
      ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(bytes[i]);
    }
    return ss.str();
  }

  bool matches(const std::string &str) const {
    return bytes == parse(str).bytes;
  }
  bool operator==(const Mac &other) const {
    return bytes == other.bytes;
  }
};

} // namespace inputtino
