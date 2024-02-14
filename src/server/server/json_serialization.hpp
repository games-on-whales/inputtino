#pragma once

#include <inputtino/input.hpp>
#include <nlohmann/json.hpp>
#include "data_model.hpp"
#include <array>
#include <ranges>
#include "utils.hpp"

using json = nlohmann::json;

void to_json(json &j, const LocalDevice &device) {
    std::string type;
    std::vector<std::string> nodes;
    switch (device.type) {
        case KEYBOARD:
            type = "KEYBOARD";
            nodes = std::get<std::shared_ptr<inputtino::Keyboard>>(device.device)->get_nodes();
            break;
        case MOUSE:
            type = "MOUSE";
            nodes = std::get<std::shared_ptr<inputtino::Mouse>>(device.device)->get_nodes();
            break;
        case JOYPAD:
            type = "JOYPAD";
            nodes = std::get<std::shared_ptr<inputtino::Joypad>>(device.device)->get_nodes();
            break;
        case PEN_TABLET:
            type = "PEN_TABLET";
            nodes = std::get<std::shared_ptr<inputtino::PenTablet>>(device.device)->get_nodes();
            break;
        case TRACKPAD:
            type = "TRACKPAD";
            nodes = std::get<std::shared_ptr<inputtino::Trackpad>>(device.device)->get_nodes();
            break;
        case TOUCH_SCREEN:
            type = "TOUCH_SCREEN";
            nodes = std::get<std::shared_ptr<inputtino::TouchScreen>>(device.device)->get_nodes();
            break;
    }


    j = json{{"device_id",    device.device_id},
             {"client_id",    device.client_id},
             {"type",         type},
             {"device_nodes", nodes}};
}

template<typename T>
void to_json(json &j, const immer::box<T> &box) {
    j = json(box.get());
}

template<typename T>
void to_json(json &j, const immer::vector<T> &vec) {
    j = json::array();
    for (const auto &v: vec) {
        j.push_back(v);
    }
}

void to_json(json &j, const devices_map &devices) {
    j = json::array();
    for (auto &[client_id, device]: devices) {
        j.push_back(json(device));
    }
}

void to_json(json &j, const ServerState &state){
    j = json{{"devices", state.devices}};
}