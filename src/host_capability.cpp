// The /dev/uhid probe and Joypad::create(), which picks a uhid or uinput pad at runtime.

#include <fcntl.h>
#include <inputtino/input.hpp>
#include <iostream>
#include <memory>
#include <unistd.h>

namespace inputtino {

bool is_uhid_supported() {
  // Probed once: it only says whether the node opens, and Joypad::create()
  // falls back to uinput anyway when creating the uhid pad fails.
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
    return {.name = "Wolf X-Box One (virtual) pad",
            // https://github.com/torvalds/linux/blob/master/drivers/input/joystick/xpad.c#L147
            .vendor_id = 0x045E,
            .product_id = 0x02EA,
            .version = 0x0408};
  case Joypad::TYPE::PS:
    return {.name = "Wolf DualSense (virtual) pad", .vendor_id = 0x054C, .product_id = 0x0CE6, .version = 0x8111};
  case Joypad::TYPE::NINTENDO:
    return {.name = "Wolf Nintendo (virtual) pad",
            // https://github.com/torvalds/linux/blob/master/drivers/hid/hid-ids.h#L981
            .vendor_id = 0x057e,
            .product_id = 0x2009,
            .version = 0x8111};
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
#ifndef INPUTTINO_NO_UHID
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
  case Joypad::TYPE::NINTENDO:
    // uinput-only; there is no uhid Nintendo backend.
    return as_joypad(SwitchJoypad::create(device));
  }
  return Error("Joypad::create: unknown Joypad::TYPE");
}

} // namespace inputtino
