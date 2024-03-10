#pragma once
#include <cstring>
#include <fcntl.h>
#include <inputtino/input.hpp>
#include <inputtino/protected_types.hpp>
#include <iostream>
#include <libudev.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <optional>
#include <thread>

namespace inputtino {
/**
 * Joypads will also have one `/dev/input/js*` device as child, we want to expose that as well
 */
static std::vector<std::string> get_child_dev_nodes(libevdev_uinput *device) {
  std::vector<std::string> result;
  auto udev = udev_new();
  if (auto device_ptr = udev_device_new_from_syspath(udev, libevdev_uinput_get_syspath(device))) {
    auto enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_parent(enumerate, device_ptr);
    udev_enumerate_scan_devices(enumerate);

    udev_list_entry *dev_list_entry;
    auto devices = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry_foreach(dev_list_entry, devices) {
      auto path = udev_list_entry_get_name(dev_list_entry);
      auto child_dev = udev_device_new_from_syspath(udev, path);
      if (auto dev_path = udev_device_get_devnode(child_dev)) {
        result.push_back(dev_path);
      }
      udev_device_unref(child_dev);
    }

    udev_enumerate_unref(enumerate);
    udev_device_unref(device_ptr);
  }

  udev_unref(udev);
  return result;
}

struct ActiveRumbleEffect {
  int effect_id;

  std::chrono::steady_clock::time_point start_point;
  std::chrono::steady_clock::time_point end_point;
  std::chrono::milliseconds length;
  ff_envelope envelope;
  struct {
    std::uint32_t weak, strong;
  } start;

  struct {
    std::uint32_t weak, strong;
  } end;
  int gain = 1;

  std::pair<std::uint32_t, std::uint32_t> previous = {0, 0};
};

static std::uint32_t rumble_magnitude(std::chrono::milliseconds time_left,
                                      std::uint32_t start,
                                      std::uint32_t end,
                                      std::chrono::milliseconds length) {
  auto rel = end - start;
  return start + (rel * time_left.count() / length.count());
}

static std::pair<std::uint32_t, std::uint32_t> simulate_rumble(const ActiveRumbleEffect &effect,
                                                               const std::chrono::steady_clock::time_point &now) {
  if (now < effect.start_point) {
    return {0, 0}; // needs to be delayed
  }

  auto time_left = std::chrono::duration_cast<std::chrono::milliseconds>(effect.end_point - now);
  auto t = effect.length - time_left;
  std::uint32_t weak = 0, strong = 0;

  if (t.count() < effect.envelope.attack_length) {
    weak = (effect.envelope.attack_level * t.count() + weak * (effect.envelope.attack_length - t.count())) /
           effect.envelope.attack_length;
    strong = (effect.envelope.attack_level * t.count() + strong * (effect.envelope.attack_length - t.count())) /
             effect.envelope.attack_length;
  } else if (time_left.count() < effect.envelope.fade_length) {
    auto dt = (t - effect.length).count() + effect.envelope.fade_length;

    weak = (effect.envelope.fade_level * dt + weak * (effect.envelope.fade_length - dt)) / effect.envelope.fade_length;
    strong = (effect.envelope.fade_level * dt + strong * (effect.envelope.fade_length - dt)) /
             effect.envelope.fade_length;
  } else {
    weak = rumble_magnitude(t, effect.start.weak, effect.end.weak, effect.length);
    strong = rumble_magnitude(t, effect.start.strong, effect.end.strong, effect.length);
  }

  weak = weak * effect.gain;
  strong = strong * effect.gain;
  return {weak, strong};
}

static ActiveRumbleEffect create_rumble_effect(int effect_id, int effect_gain, const ff_effect &effect) {
  // All duration values are expressed in ms. Values above 32767 ms (0x7fff) should not be used
  auto delay = std::chrono::milliseconds{std::clamp(effect.replay.delay, (__u16)0, (__u16)32767)};
  auto length = std::chrono::milliseconds{std::clamp(effect.replay.length, (__u16)0, (__u16)32767)};
  auto now = std::chrono::steady_clock::now();
  ActiveRumbleEffect r_effect{.effect_id = effect_id,
                              .start_point = now + delay,
                              .end_point = now + delay + length,
                              .length = length,
                              .envelope = {},
                              .gain = effect_gain};
  switch (effect.type) {
  case FF_CONSTANT:
    r_effect.start.weak = effect.u.constant.level;
    r_effect.start.strong = effect.u.constant.level;
    r_effect.end.weak = effect.u.constant.level;
    r_effect.end.strong = effect.u.constant.level;
    r_effect.envelope = effect.u.constant.envelope;
    break;
  case FF_PERIODIC:
    r_effect.start.weak = effect.u.periodic.magnitude;
    r_effect.start.strong = effect.u.periodic.magnitude;
    r_effect.end.weak = effect.u.periodic.magnitude;
    r_effect.end.strong = effect.u.periodic.magnitude;
    r_effect.envelope = effect.u.periodic.envelope;
    break;
  case FF_RAMP:
    r_effect.start.weak = effect.u.ramp.start_level;
    r_effect.start.strong = effect.u.ramp.start_level;
    r_effect.end.weak = effect.u.ramp.end_level;
    r_effect.end.strong = effect.u.ramp.end_level;
    r_effect.envelope = effect.u.ramp.envelope;
    break;
  case FF_RUMBLE:
    r_effect.start.weak = effect.u.rumble.weak_magnitude;
    r_effect.start.strong = effect.u.rumble.strong_magnitude;
    r_effect.end.weak = effect.u.rumble.weak_magnitude;
    r_effect.end.strong = effect.u.rumble.strong_magnitude;
    break;
  }
  return r_effect;
}

/**
 * Here we listen for events from the device and call the corresponding callback functions
 *
 * Rumble:
 *   First of, this is called force feedback (FF) in linux,
 *   you can see some docs here: https://www.kernel.org/doc/html/latest/input/ff.html
 *   In uinput this works as a two step process:
 *    - you first upload the FF effect with a given request ID
 *    - later on when the rumble has been activated you'll receive an EV_FF in your /dev/input/event**
 *      where the value is the request ID
 *   You can test the virtual devices that we create by simply using the utility `fftest`
 */
static void event_listener(const std::shared_ptr<BaseJoypadState> &state) {
  std::this_thread::sleep_for(100ms); // We have to sleep in order to be able to read from the newly created device

  auto uinput_fd = libevdev_uinput_get_fd(state->joy.get());
  if (uinput_fd < 0) {
    std::cerr << "Unable to open uinput device, additional events will be disabled.";
    return;
  }

  // We have to add 0_NONBLOCK to the flags in order to be able to read the events
  int flags = fcntl(uinput_fd, F_GETFL, 0);
  fcntl(uinput_fd, F_SETFL, flags | O_NONBLOCK);

  /* Local copy of all the uploaded ff effects */
  std::map<int, ff_effect> ff_effects = {};

  /* Currently running ff effects */
  std::vector<ActiveRumbleEffect> active_effects = {};

  auto remove_effects = [&](auto filter_fn) {
    active_effects.erase(std::remove_if(active_effects.begin(),
                                        active_effects.end(),
                                        [&](const auto effect) {
                                          auto to_be_removed = filter_fn(effect);
                                          if (to_be_removed && state->on_rumble) {
                                            state->on_rumble.value()(0, 0);
                                          }
                                          return to_be_removed;
                                        }),
                         active_effects.end());
  };

  while (!state->stop_listening_events) {
    std::this_thread::sleep_for(20ms); // TODO: configurable?

    int effect_gain = 1;

    auto events = fetch_events(uinput_fd);
    for (auto ev : events) {
      if (ev->type == EV_UINPUT && ev->code == UI_FF_UPLOAD) { // Upload a new FF effect
        uinput_ff_upload upload{};
        upload.request_id = ev->value;

        ioctl(uinput_fd, UI_BEGIN_FF_UPLOAD, &upload); // retrieve the effect

        ff_effects.insert_or_assign(upload.effect.id, upload.effect);
        upload.retval = 0;

        ioctl(uinput_fd, UI_END_FF_UPLOAD, &upload);
      } else if (ev->type == EV_UINPUT && ev->code == UI_FF_ERASE) { // Remove an uploaded FF effect
        uinput_ff_erase erase{};
        erase.request_id = ev->value;

        ioctl(uinput_fd, UI_BEGIN_FF_ERASE, &erase); // retrieve ff_erase

        ff_effects.erase(erase.effect_id);
        erase.retval = 0;

        ioctl(uinput_fd, UI_END_FF_ERASE, &erase);
      } else if (ev->type == EV_FF && ev->code == FF_GAIN) { // Force feedback set gain
        effect_gain = std::clamp(ev->value, 0, 0xFFFF);
      } else if (ev->type == EV_FF) { // Force feedback effect
        auto effect_id = ev->code;
        if (ev->value) { // Activate
          if (ff_effects.find(effect_id) != ff_effects.end() && state->on_rumble) {
            auto effect = ff_effects[effect_id];
            active_effects.emplace_back(create_rumble_effect(effect_id, effect_gain, effect));
          }
        } else { // Deactivate
          remove_effects([effect_id](const auto &effect) { return effect.effect_id == effect_id; });
        }
      } else if (ev->type == EV_LED) {
        // TODO: support LED
      }
    }

    auto now = std::chrono::steady_clock::now();

    // Remove effects that have ended
    remove_effects([now](const auto effect) { return effect.end_point <= now; });

    // Simulate rumble
    for (auto effect : active_effects) {
      auto [weak, strong] = simulate_rumble(effect, now);
      if (effect.previous.first != weak || effect.previous.second != strong) {
        effect.previous.first = weak;
        effect.previous.second = strong;

        if (auto callback = state->on_rumble) {
          callback.value()(weak, strong);
        }
      }
    }
  }
}

} // namespace inputtino