// Runtime probes for host-level input capabilities plus the runtime joypad
// factory, used by callers that want to pick between uhid- and uinput-backed
// virtual devices based on what the kernel actually exposes here.

#include <fcntl.h>
#include <inputtino/input.hpp>
#include <iostream>
#include <memory>
#include <unistd.h>

namespace inputtino {

bool is_uhid_supported() {
  // Cached once per process — UHID node presence doesn't change at
  // runtime on any sane host, and probing on every controller
  // creation would add ~200us of open/close per join.
  static const bool supported = []() {
    int fd = open("/dev/uhid", O_RDWR | O_CLOEXEC);
    if (fd < 0) {
      return false;
    }
    close(fd);
    return true;
  }();
  return supported;
}

namespace {

// Wrap a concrete Result<T> into a Result<unique_ptr<Joypad>>, moving the
// created pad onto the heap and up-casting to the Joypad base.
template <typename T> Result<std::unique_ptr<Joypad>> as_joypad(Result<T> created) {
  if (!created) {
    return Error(created.getErrorMessage());
  }
  return std::unique_ptr<Joypad>(std::make_unique<T>(std::move(*created)));
}

} // namespace

DeviceDefinition Joypad::default_definition(Joypad::TYPE kind) {
  switch (kind) {
  case Joypad::TYPE::XBOX:
    return {.name = "Wolf X-Box One (virtual) pad", .vendor_id = 0x045E, .product_id = 0x02EA, .version = 0x0408};
  case Joypad::TYPE::PS:
    return {.name = "Wolf DualSense (virtual) pad", .vendor_id = 0x054C, .product_id = 0x0CE6, .version = 0x8111};
  case Joypad::TYPE::JOYCON_LEFT:
    // hid-nintendo keys the Joy-Con side off the product id (0x2006 = left).
    return {.name = "Wolf Joy-Con (L) (virtual) pad", .vendor_id = 0x057e, .product_id = 0x2006, .version = 0x8111};
  case Joypad::TYPE::JOYCON_RIGHT:
    return {.name = "Wolf Joy-Con (R) (virtual) pad", .vendor_id = 0x057e, .product_id = 0x2007, .version = 0x8111};
  case Joypad::TYPE::NINTENDO:
    return {.name = "Wolf Nintendo (virtual) pad", .vendor_id = 0x057e, .product_id = 0x2009, .version = 0x8111};
  }
  return {};
}

Result<std::unique_ptr<Joypad>> Joypad::create(Joypad::TYPE kind, bool prefer_uhid) {
  return create(kind, default_definition(kind), prefer_uhid);
}

Result<std::unique_ptr<Joypad>> Joypad::create(Joypad::TYPE kind, const DeviceDefinition &device, bool prefer_uhid) {
  switch (kind) {
  case Joypad::TYPE::XBOX:
    // uinput-only; there is no uhid Xbox backend.
    return as_joypad(XboxOneJoypad::create(device));
  case Joypad::TYPE::PS:
#ifdef INPUTTINO_USE_UHID
    if (prefer_uhid) {
      if (auto pad = PS5Joypad::create(device)) {
        return std::unique_ptr<Joypad>(std::make_unique<PS5Joypad>(std::move(*pad)));
      } else {
        std::cerr << "inputtino: uhid PS5 joypad creation failed (" << pad.getErrorMessage()
                  << "), falling back to the uinput backend" << std::endl;
      }
    }
#else
    (void)prefer_uhid;
#endif
    return as_joypad(PS5JoypadUinput::create(device));
  // Pro Controller and the two Joy-Cons are the same hid-nintendo pad; the
  // SwitchJoypad picks Pro / L / R from device.product_id (see default_definition).
  case Joypad::TYPE::JOYCON_LEFT:
  case Joypad::TYPE::JOYCON_RIGHT:
  case Joypad::TYPE::NINTENDO:
#ifdef INPUTTINO_USE_UHID
    if (prefer_uhid) {
      if (auto pad = SwitchJoypad::create(device)) {
        return std::unique_ptr<Joypad>(std::make_unique<SwitchJoypad>(std::move(*pad)));
      } else {
        std::cerr << "inputtino: uhid Switch joypad creation failed (" << pad.getErrorMessage()
                  << "), falling back to the uinput backend" << std::endl;
      }
    }
#else
    (void)prefer_uhid;
#endif
    return as_joypad(SwitchJoypadUinput::create(device));
  }
  return Error("Joypad::create: unknown Joypad::TYPE");
}

} // namespace inputtino
