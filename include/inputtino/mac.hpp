#pragma once

#include <array>
#include <chrono>
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

  /**
   * Use a caller-supplied address (e.g. DeviceDefinition::device_uniq) for a new
   * virtual device.
   *
   * Unlike generate(), the same address can be passed on every create, so the pad
   * keeps a stable identity across re-creates (Steam, for example, keys Bluetooth
   * DualSense configs by address, so a fresh address looks like a brand-new pad).
   *
   * The kernel rejects a device whose address is still used by another one, which
   * is exactly what happens when a pad is re-created with the same address before
   * the previous device has gone away. This waits up to `timeout` for the address
   * to be released and returns an Error if it is still in use afterwards.
   */
  static Result<Mac> claim(const std::string &str, std::chrono::milliseconds timeout = std::chrono::milliseconds(500));

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
