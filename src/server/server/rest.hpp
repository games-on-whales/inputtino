#pragma once

#include <httplib.h>
#include <server/json_serialization.hpp>
#include <immer/atom.hpp>
#include <memory>
#include <string>
#include <vector>
#include <functional>

static void handle_error(httplib::Response &res, const std::string &message) {
    res.set_content(json{{"error", message}}.dump(), "application/json");
    res.status = httplib::StatusCode::InternalServerError_500;
}

template<typename T>
static bool
handle_device(inputtino::Result<std::shared_ptr<T>> &result, httplib::Response &response,
              LocalDevice &device) {
    if (!result) {
        handle_error(response, result.getErrorMessage());
        return false;
    } else {
        device.device_id = std::hash<std::string>{}((*result)->get_nodes()[0]);
        device.device = std::move(*result);
        return true;
    }
}

std::unique_ptr<httplib::Server> setup_rest_server(std::shared_ptr<immer::atom<ServerState>> state) {
    auto svr = std::make_unique<httplib::Server>();

    svr->Get("/api/v1.0/devices", [state](const httplib::Request &, httplib::Response &res) {
        res.set_content(json(state->load()).dump(), "application/json");
    });

    svr->Post("/api/v1.0/devices/add", [state](const httplib::Request &req, httplib::Response &res) {
        auto payload = json::parse(req.body);

        state->update([&](const ServerState &state) {
            auto device_type = to_lower((std::string) payload.at("type"));
            ServerState new_state = state;
            LocalDevice new_device = {.client_id = req.remote_addr};
            bool success = false;
            switch (hash(device_type)) {
                case hash("keyboard"): {
                    auto keyboard = inputtino::Keyboard::create();
                    success = handle_device(keyboard, res, new_device);
                    new_device.type = DeviceType::KEYBOARD;
                    break;
                }
                case hash("joypad"): {
                    auto joypad_type = to_lower((std::string) payload.value("joypad_type", "xbox"));
                    inputtino::Joypad::CONTROLLER_TYPE type;
                    switch (hash(joypad_type)) {
                        case hash("xbox"):
                            type = inputtino::Joypad::CONTROLLER_TYPE::XBOX;
                            break;
                        case hash("ps"):
                            type = inputtino::Joypad::CONTROLLER_TYPE::PS;
                            break;
                        case hash("nintendo"):
                            type = inputtino::Joypad::CONTROLLER_TYPE::NINTENDO;
                            break;
                        default:
                            handle_error(res, "Unknown joypad type: " + joypad_type);
                    }

                    // TODO: ["ANALOG_TRIGGERS, "RUMBLE"] --> uint8_t
                    uint8_t capabilities = payload.value("capabilities", 0);

                    auto joypad = inputtino::Joypad::create(type, capabilities);
                    success = handle_device(joypad, res, new_device);
                    new_device.type = DeviceType::JOYPAD;
                    break;
                }
                case hash("mouse"): {
                    auto mouse = inputtino::Mouse::create();
                    success = handle_device(mouse, res, new_device);
                    new_device.type = DeviceType::MOUSE;
                    break;
                }
                case hash("touchscreen"): {
                    auto touch = inputtino::TouchScreen::create();
                    success = handle_device(touch, res, new_device);
                    new_device.type = DeviceType::TOUCH_SCREEN;
                    break;
                }
                case hash("pen_tablet"): {
                    auto pen = inputtino::PenTablet::create();
                    success = handle_device(pen, res, new_device);
                    new_device.type = DeviceType::PEN_TABLET;
                    break;
                }
                case hash("trackpad"): {
                    auto trackpad = inputtino::Trackpad::create();
                    success = handle_device(trackpad, res, new_device);
                    new_device.type = DeviceType::TRACKPAD;
                    break;
                }
                default:
                    handle_error(res, "Unknown device type: " + device_type);
                    break;
            }

            if (success) {
                res.set_content(json(new_device).dump(), "application/json");
                new_state.devices = state.devices.set(
                        new_device.device_id,
                        std::move(new_device));
            }

            return new_state;
        });
    });

    svr->Delete("/api/v1.0/devices/:id", [state](const httplib::Request &req, httplib::Response &res) {
        std::size_t id = std::stoul(req.path_params.at("id"));
        state->update([id](const ServerState &state) {
            return ServerState{.devices = state.devices.erase(id)};
        });
        res.set_content(json{{"success", true}}.dump(), "application/json");
    });

    /* Mouse handlers */
    svr->Post("/api/v1.0/devices/mouse/:id/move_rel", [state](const httplib::Request &req, httplib::Response &res) {
        std::size_t id = std::stoul(req.path_params.at("id"));
        auto payload = json::parse(req.body);
        double delta_x = payload.value("delta_x", 0.0);
        double delta_y = payload.value("delta_y", 0.0);
        auto device = state->load()->devices.at(id);
        auto mouse = std::get<std::shared_ptr<inputtino::Mouse>>(device->device);
        mouse->move(delta_x, delta_y);
        res.set_content(json{{"success", true}}.dump(), "application/json");
    });

    // TODO: implement all other handlers...

    /* Default error handling */
    svr->set_exception_handler([](const auto &req, auto &res, std::exception_ptr ep) {
        std::string error_msg = "Internal server error";
        try {
            std::rethrow_exception(ep);
        } catch (std::exception &e) {
            std::cerr << "Exception when calling " << req.path << " : " << e.what() << std::endl;
            error_msg = e.what();
        } catch (...) { // See NOTE in GitHub README
        }
        handle_error(res, error_msg);
    });

    return svr;
}