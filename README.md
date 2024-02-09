# inputtino

An easy to use virtual input library for Linux built on top of uinput and evdev.  
Supports:

- Keyboard
- Mouse
- Touchscreen
- Trackpad
- Pen tablet
- Joypad
  - Correctly emulates Xbox, PS5 or Nintendo joypads
  - Supports callbacks on Rumble events
  - [WIP] Gyro and Acceleration support

## Example usage

```c++
auto joypad = Joypad::create(Joypad::PS, Joypad::RUMBLE | Joypad::ANALOG_TRIGGERS);

joypad->set_stick(Joypad::LS, 1000, 2000);
joypad->set_pressed_buttons(Joypad::X | Joypad::DPAD_RIGHT);

auto rumble_data = std::make_shared<std::pair<int, int>>();
joypad.set_on_rumble([rumble_data](int low_freq, int high_freq) {
    rumble_data->first = low_freq;
    rumble_data->second = high_freq;
});
```

For more examples you can look at the unit tests under `tests/`: Joypads have been tested using `SDL2` other input
devices have been tested with `libinput`.