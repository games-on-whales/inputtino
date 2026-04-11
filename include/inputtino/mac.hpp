#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace inputtino {

/**
 * A 6-byte MAC address for inputtino virtual devices, with a small allocation
 * pool so each virtual device gets a unique address (the kernel rejects
 * duplicate MACs). The implementation lives in mac.cpp to keep this public
 * header free of heavy includes.
 */
struct Mac {
  std::array<unsigned char, 6> bytes = {};

  /**
   * Allocate the next free MAC with the inputtino prefix AA:BB:CC:00:xx:xx,
   * avoiding collisions with both the internal pool and existing UHID devices.
   * Thread-safe. Call release() when the device is destroyed.
   */
  static Mac generate();

  /** Return a MAC to the pool so it can be reused. */
  static void release(const Mac &mac);

  /** Parse a "xx:xx:xx:xx:xx:xx" string. */
  static Mac parse(const std::string &str);

  /** Format as "xx:xx:xx:xx:xx:xx". */
  std::string to_string() const;

  bool matches(const std::string &str) const {
    return bytes == parse(str).bytes;
  }
  bool operator==(const Mac &other) const {
    return bytes == other.bytes;
  }
  bool operator!=(const Mac &other) const {
    return bytes != other.bytes;
  }
};

} // namespace inputtino
