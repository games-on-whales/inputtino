#pragma once

#include <immer/map.hpp>
#include <immer/vector.hpp>
#include <immer/box.hpp>
#include <inputtino/input.hpp>

enum DeviceType {
    KEYBOARD,
    MOUSE,
    JOYPAD,
    PEN_TABLET,
    TRACKPAD,
    TOUCH_SCREEN
};

struct LocalDevice {
    DeviceType type;
    std::size_t device_id;
    std::string client_id;
    std::variant<
            std::shared_ptr<inputtino::Keyboard>,
            std::shared_ptr<inputtino::Mouse>,
            std::shared_ptr<inputtino::Joypad>,
            std::shared_ptr<inputtino::PenTablet>,
            std::shared_ptr<inputtino::Trackpad>,
            std::shared_ptr<inputtino::TouchScreen>>
            device;
};

using devices_map = immer::map<std::size_t, immer::box<LocalDevice>>;

struct ServerState {
    devices_map devices;
};