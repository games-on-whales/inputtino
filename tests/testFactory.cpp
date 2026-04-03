#include <catch2/catch_test_macros.hpp>
#include <inputtino/input.hpp>

using namespace inputtino;

// These cover the runtime backend selection, the basic uinput joypad backends
// and the self-describing udev metadata. They deliberately avoid the SDL/HIDAPI
// round-trip (see testUHID/testJoypads) so they stay deterministic.

TEST_CASE("is_uhid_supported is stable across calls", "[Factory]") {
  const bool supported = is_uhid_supported();
  REQUIRE(is_uhid_supported() == supported);
}

TEST_CASE("uinput DualSense pad: no rich features, safe no-ops, self-describes", "[Factory]") {
  auto created = PS5JoypadUinput::create();
  REQUIRE(created);
  auto joypad = std::move(*created);

  // The uinput backend has no motion; the rich setters inherited from the
  // Joypad base must be safe no-ops rather than crash.
  REQUIRE_FALSE(joypad.supports_motion());
  joypad.set_accel(1.f, 2.f, 3.f);
  joypad.set_gyro(1.f, 2.f, 3.f);
  joypad.set_battery(Joypad::BATTERY_FULL, 100);
  joypad.place_finger(0, 10, 10);
  joypad.release_finger(0);

  // It still presents at least one joystick node to udev.
  auto events = joypad.get_udev_events();
  REQUIRE_FALSE(events.empty());
  bool has_joystick = false;
  for (const auto &ev : events) {
    auto it = ev.find("ID_INPUT_JOYSTICK");
    if (it != ev.end() && it->second == "1") {
      has_joystick = true;
    }
  }
  REQUIRE(has_joystick);
  REQUIRE_FALSE(joypad.get_udev_hw_db_entries().empty());
}

TEST_CASE("uinput Nintendo pad: no motion, self-describes", "[Factory]") {
  auto created = SwitchJoypadUinput::create();
  REQUIRE(created);
  auto joypad = std::move(*created);

  REQUIRE_FALSE(joypad.supports_motion());
  joypad.set_gyro(0.f, 0.f, 0.f); // no-op, must not crash
  REQUIRE_FALSE(joypad.get_udev_events().empty());
}

TEST_CASE("Joypad::create returns a usable pad through the Joypad base", "[Factory]") {
  // prefer_uhid = false always yields the uinput backend, which has no motion.
  auto ps = Joypad::create(Joypad::TYPE::PS, /* prefer_uhid */ false);
  REQUIRE(ps);
  REQUIRE_FALSE((*ps)->supports_motion());
  REQUIRE_FALSE((*ps)->get_udev_events().empty());

  auto nintendo = Joypad::create(Joypad::TYPE::NINTENDO, /* prefer_uhid */ false);
  REQUIRE(nintendo);
  REQUIRE_FALSE((*nintendo)->supports_motion());

  auto xbox = Joypad::create(Joypad::TYPE::XBOX);
  REQUIRE(xbox);
  REQUIRE_FALSE((*xbox)->supports_motion());
  REQUIRE_FALSE((*xbox)->get_udev_events().empty());
}

#ifdef USE_UHID
TEST_CASE("Joypad::create picks the rich uhid backend when preferred", "[Factory][UHID]") {
  if (!is_uhid_supported()) {
    SKIP("This host has no accessible /dev/uhid");
  }

  // The uhid DualSense / Switch Pro pads forward motion; that's the observable
  // difference from their uinput fallbacks.
  auto ps = Joypad::create(Joypad::TYPE::PS, /* prefer_uhid */ true);
  REQUIRE(ps);
  REQUIRE((*ps)->supports_motion());

  auto nintendo = Joypad::create(Joypad::TYPE::NINTENDO, /* prefer_uhid */ true);
  REQUIRE(nintendo);
  REQUIRE((*nintendo)->supports_motion());
}
#endif
