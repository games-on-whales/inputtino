#pragma once

#include <array>
#include <cstdint>
#include <inputtino/result.hpp>
#include <string>

namespace inputtino {

/**
 * A 6-byte MAC address for inputtino virtual devices. Addresses are handed
 * out from a monotonically increasing counter (each virtual device needs a
 * unique address; the kernel rejects duplicate MACs).
 */
struct Mac {
  std::array<unsigned char, 6> bytes = {};

  /**
   * Allocate the next free MAC with the inputtino prefix AA:BB:CC:00:xx:xx,
   * skipping any address already active on the system. Thread-safe.
   */
  static Result<Mac> generate();

  /** Parse a "xx:xx:xx:xx:xx:xx" string. */
  static Result<Mac> parse(const std::string &str);

  /** Format as "xx:xx:xx:xx:xx:xx". */
  std::string to_string() const;

  bool matches(const std::string &str) const {
    auto parsed = parse(str);
    return parsed && bytes == (*parsed).bytes;
  }
  bool operator==(const Mac &other) const {
    return bytes == other.bytes;
  }
  bool operator!=(const Mac &other) const {
    return bytes != other.bytes;
  }
};

} // namespace inputtino
