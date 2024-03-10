#include "catch2/catch_all.hpp"
#include <inputtino/input.hpp>
#include <iostream>
#include <SDL.h>
#include <thread>

using Catch::Matchers::Equals;
using Catch::Matchers::WithinAbs;
using namespace inputtino;

void flush_sdl_events() {
  SDL_JoystickUpdate();
  SDL_Event event;
  while (SDL_PollEvent(&event) != 0) {
    switch (event.type) {
    case SDL_CONTROLLERDEVICEADDED:
      std::cout << "SDL_CONTROLLERDEVICEADDED " << SDL_GameControllerNameForIndex(event.cdevice.which) << std::endl;
      break;
    case SDL_CONTROLLERDEVICEREMOVED:
      std::cout << "SDL_CONTROLLERDEVICEREMOVED " << event.cdevice.which << std::endl;
      break;
    case SDL_CONTROLLERDEVICEREMAPPED:
      std::cout << "SDL_CONTROLLERDEVICEREMAPPED " << SDL_GameControllerNameForIndex(event.cdevice.which) << std::endl;
      break;
    }
  }
}

class SDLTestsFixture {
public:
  SDLTestsFixture() {
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
    SDL_GameControllerEventState(SDL_ENABLE);
  }

  ~SDLTestsFixture() {
    SDL_Quit();
  }
};

#define SDL_TEST_BUTTON(JOYPAD_BTN, SDL_BTN)                                                                           \
  REQUIRE(SDL_GameControllerGetButton(gc, SDL_BTN) == 0);                                                              \
  joypad.set_pressed_buttons(JOYPAD_BTN);                                                                              \
  flush_sdl_events();                                                                                                  \
  REQUIRE(SDL_GameControllerGetButton(gc, SDL_BTN) == 1);

void test_buttons(SDL_GameController *gc, Joypad &joypad) {
  SDL_TEST_BUTTON(Joypad::DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_UP);
  SDL_TEST_BUTTON(Joypad::DPAD_DOWN, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
  SDL_TEST_BUTTON(Joypad::DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
  SDL_TEST_BUTTON(Joypad::DPAD_RIGHT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);

  SDL_TEST_BUTTON(Joypad::LEFT_STICK, SDL_CONTROLLER_BUTTON_LEFTSTICK);
  SDL_TEST_BUTTON(Joypad::RIGHT_STICK, SDL_CONTROLLER_BUTTON_RIGHTSTICK);
  SDL_TEST_BUTTON(Joypad::LEFT_BUTTON, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
  SDL_TEST_BUTTON(Joypad::RIGHT_BUTTON, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);

  SDL_TEST_BUTTON(Joypad::A, SDL_CONTROLLER_BUTTON_A);
  SDL_TEST_BUTTON(Joypad::B, SDL_CONTROLLER_BUTTON_B);
  SDL_TEST_BUTTON(Joypad::X, SDL_CONTROLLER_BUTTON_X);
  SDL_TEST_BUTTON(Joypad::Y, SDL_CONTROLLER_BUTTON_Y);

  SDL_TEST_BUTTON(Joypad::START, SDL_CONTROLLER_BUTTON_START);
  SDL_TEST_BUTTON(Joypad::BACK, SDL_CONTROLLER_BUTTON_BACK);
  SDL_TEST_BUTTON(Joypad::HOME, SDL_CONTROLLER_BUTTON_GUIDE);

  // Release all buttons
  joypad.set_pressed_buttons(0);
  flush_sdl_events();
  REQUIRE(SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_A) == 0);
  REQUIRE(SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_B) == 0);
  REQUIRE(SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_X) == 0);
  REQUIRE(SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_Y) == 0);

  // Press some of them together
  joypad.set_pressed_buttons(Joypad::A | Joypad::B | Joypad::X | Joypad::Y);
  flush_sdl_events();
  REQUIRE(SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_A) == 1);
  REQUIRE(SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_B) == 1);
  REQUIRE(SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_X) == 1);
  REQUIRE(SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_Y) == 1);
}

TEST_CASE_METHOD(SDLTestsFixture, "PS Joypad", "[SDL]") {
  // Create the controller
  auto joypad_ptr = *PS5Joypad::create();
  auto joypad = *joypad_ptr;

  std::this_thread::sleep_for(250ms);

  SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5, "1");
  SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE, "1");
  SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5_PLAYER_LED, "1");
  // Initializing the controller
  flush_sdl_events();
  SDL_GameController *gc = SDL_GameControllerOpen(0);
  if (gc == nullptr) {
    WARN(SDL_GetError());
  }
  REQUIRE(gc);

  REQUIRE(SDL_GameControllerGetType(gc) == SDL_CONTROLLER_TYPE_PS5);

  { // Rumble
    // Checking for basic capability
    REQUIRE(SDL_GameControllerHasRumble(gc));

    auto rumble_data = std::make_shared<std::pair<int, int>>();
    joypad.set_on_rumble([rumble_data](int low_freq, int high_freq) {
      rumble_data->first = low_freq;
      rumble_data->second = high_freq;
    });

    // When debugging this, bear in mind that SDL will send max duration here
    // https://github.com/libsdl-org/SDL/blob/da8fc70a83cf6b76d5ea75c39928a7961bd163d3/src/joystick/linux/SDL_sysjoystick.c#L1628
    SDL_GameControllerRumble(gc, 0xFF00, 0xF00F, 100);
    std::this_thread::sleep_for(30ms); // wait for the effect to be picked up
    REQUIRE(rumble_data->first == 0xFFFF);
    REQUIRE(rumble_data->second == 0xF0F0);
  }

  { // LED
    // Unfortunately LINUX_JoystickSetLED is not implemented in SDL2
    //        REQUIRE(SDL_GameControllerHasLED(gc));
    //        struct LED {
    //          int r;
    //          int g;
    //          int b;
    //        };
    //        auto led_data = std::make_shared<LED>();
    //        joypad.set_on_led([led_data](int r, int g, int b) {
    //          led_data->r = r;
    //          led_data->g = g;
    //          led_data->b = b;
    //        });
    //        SDL_GameControllerSetLED(gc, 50, 100, 150);
    //        std::this_thread::sleep_for(30ms); // wait for the effect to be picked up
    //        REQUIRE(led_data->r == 50);
    //        REQUIRE(led_data->g == 100);
    //        REQUIRE(led_data->b == 150);
  }

  test_buttons(gc, joypad); // TODO: fix failing buttons
  {                         // Sticks
    REQUIRE(SDL_GameControllerHasAxis(gc, SDL_CONTROLLER_AXIS_LEFTX));
    REQUIRE(SDL_GameControllerHasAxis(gc, SDL_CONTROLLER_AXIS_LEFTY));
    REQUIRE(SDL_GameControllerHasAxis(gc, SDL_CONTROLLER_AXIS_RIGHTX));
    REQUIRE(SDL_GameControllerHasAxis(gc, SDL_CONTROLLER_AXIS_RIGHTY));
    REQUIRE(SDL_GameControllerHasAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERLEFT));
    REQUIRE(SDL_GameControllerHasAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERRIGHT));

    joypad.set_stick(Joypad::LS, 1000, 2000);
    flush_sdl_events();
    REQUIRE(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTX) == 899);
    REQUIRE(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTY) == 1927);

    joypad.set_stick(Joypad::RS, -16384, -32768);
    flush_sdl_events();
    REQUIRE(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_RIGHTX) == -16320);
    REQUIRE(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_RIGHTY) == -32768);

    joypad.set_triggers(125, 255);
    flush_sdl_events();
    REQUIRE(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERLEFT) == 16062);
    REQUIRE(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) == 32767);

    joypad.set_triggers(0, 0);
    flush_sdl_events();
    REQUIRE(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERLEFT) == 0);
    REQUIRE(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) == 0);
  }
  { // test acceleration
    REQUIRE(SDL_GameControllerHasSensor(gc, SDL_SENSOR_ACCEL));
    if (SDL_GameControllerSetSensorEnabled(gc, SDL_SENSOR_ACCEL, SDL_TRUE) != 0) {
      WARN(SDL_GetError());
    }

    std::array<float, 3> acceleration_data = {9.8f, 0.0f, 20.0f};
    joypad.set_motion(inputtino::PS5Joypad::ACCELERATION,
                      acceleration_data[0],
                      acceleration_data[1],
                      acceleration_data[2]);
    SDL_GameControllerUpdate();
    SDL_SensorUpdate();
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_CONTROLLERSENSORUPDATE) {
        break;
      }
    }
    REQUIRE(event.type == SDL_CONTROLLERSENSORUPDATE);
    REQUIRE(event.csensor.sensor == SDL_SENSOR_ACCEL);
    REQUIRE_THAT(event.csensor.data[0], WithinAbs(acceleration_data[0], 0.9f));
    REQUIRE_THAT(event.csensor.data[1], WithinAbs(acceleration_data[1], 0.9f));
    REQUIRE_THAT(event.csensor.data[2], WithinAbs(acceleration_data[2], 0.9f));
    flush_sdl_events();
  }
  { // test gyro
    REQUIRE(SDL_GameControllerHasSensor(gc, SDL_SENSOR_GYRO));
    if (SDL_GameControllerSetSensorEnabled(gc, SDL_SENSOR_GYRO, SDL_TRUE) != 0) {
      WARN(SDL_GetError());
    }

    std::array<float, 3> gyro_data = {0.0f, 10.0f, 20.0f};
    joypad.set_motion(inputtino::PS5Joypad::GYROSCOPE, gyro_data[0], gyro_data[1], gyro_data[2]);
    std::this_thread::sleep_for(10ms);
    joypad.set_motion(inputtino::PS5Joypad::GYROSCOPE, gyro_data[0], gyro_data[1], gyro_data[2]);

    SDL_GameControllerUpdate();
    SDL_SensorUpdate();
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_CONTROLLERSENSORUPDATE) {
        break;
      }
    }
    REQUIRE(event.type == SDL_CONTROLLERSENSORUPDATE);
    REQUIRE(event.csensor.sensor == SDL_SENSOR_GYRO);
    REQUIRE_THAT(event.csensor.data[0], WithinAbs(gyro_data[0], 0.001f));
    REQUIRE_THAT(event.csensor.data[1], WithinAbs(gyro_data[1], 0.001f));
    REQUIRE_THAT(event.csensor.data[2], WithinAbs(gyro_data[2], 0.001f));
  }
  // TODO: test touchpad
  // TODO: test battery
  // Adaptive triggers aren't supported by SDL
  // see:https://github.com/libsdl-org/SDL/issues/5125#issuecomment-1204261666

  SDL_GameControllerClose(gc);
}

TEST_CASE_METHOD(SDLTestsFixture, "XBOX Joypad", "[SDL]") {
  // Create the controller
  auto joypad_ptr = *XboxOneJoypad::create();
  auto joypad = *joypad_ptr;

  std::this_thread::sleep_for(150ms);

  // Initializing the controller
  flush_sdl_events();
  SDL_GameController *gc = SDL_GameControllerOpen(0);
  if (gc == nullptr) {
    WARN(SDL_GetError());
  }
  REQUIRE(gc);
  REQUIRE(SDL_GameControllerGetType(gc) == SDL_CONTROLLER_TYPE_XBOXONE);
  // Checking for basic joypad capabilities
  REQUIRE(SDL_GameControllerHasRumble(gc));

  test_buttons(gc, joypad);
  { // Rumble
    // Checking for basic capability
    REQUIRE(SDL_GameControllerHasRumble(gc));

    auto rumble_data = std::make_shared<std::pair<int, int>>();
    joypad.set_on_rumble([rumble_data](int low_freq, int high_freq) {
      rumble_data->first = low_freq;
      rumble_data->second = high_freq;
    });

    // When debugging this, bear in mind that SDL will send max duration here
    // https://github.com/libsdl-org/SDL/blob/da8fc70a83cf6b76d5ea75c39928a7961bd163d3/src/joystick/linux/SDL_sysjoystick.c#L1628
    SDL_GameControllerRumble(gc, 100, 200, 100);
    std::this_thread::sleep_for(30ms); // wait for the effect to be picked up
    REQUIRE(rumble_data->first == 200);
    REQUIRE(rumble_data->second == 100);
  }

  { // Sticks
    REQUIRE(SDL_GameControllerHasAxis(gc, SDL_CONTROLLER_AXIS_LEFTX));
    REQUIRE(SDL_GameControllerHasAxis(gc, SDL_CONTROLLER_AXIS_LEFTY));
    REQUIRE(SDL_GameControllerHasAxis(gc, SDL_CONTROLLER_AXIS_RIGHTX));
    REQUIRE(SDL_GameControllerHasAxis(gc, SDL_CONTROLLER_AXIS_RIGHTY));
    REQUIRE(SDL_GameControllerHasAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERLEFT));
    REQUIRE(SDL_GameControllerHasAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERRIGHT));

    joypad.set_stick(Joypad::LS, 1000, 2000);
    flush_sdl_events();
    REQUIRE(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTX) == 1000);
    REQUIRE(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTY) == -2000);

    joypad.set_stick(Joypad::RS, 1000, 2000);
    flush_sdl_events();
    REQUIRE(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_RIGHTX) == 1000);
    REQUIRE(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_RIGHTY) == -2000);

    joypad.set_triggers(10, 20);
    flush_sdl_events();
    REQUIRE(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERLEFT) == 1284);
    REQUIRE(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) == 2569);

    joypad.set_triggers(0, 0);
    flush_sdl_events();
    REQUIRE(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERLEFT) == 0);
    REQUIRE(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) == 0);
  }

  SDL_GameControllerClose(gc);
}

TEST_CASE_METHOD(SDLTestsFixture, "Nintendo Joypad", "[SDL]") {
  // Create the controller
  auto joypad_ptr = *SwitchJoypad::create();
  auto joypad = *joypad_ptr;

  std::this_thread::sleep_for(150ms);

  // Initializing the controller
  flush_sdl_events();
  SDL_GameController *gc = SDL_GameControllerOpen(0);
  if (gc == nullptr) {
    WARN(SDL_GetError());
  }
  REQUIRE(gc);
  REQUIRE(SDL_GameControllerGetType(gc) == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO);

  test_buttons(gc, joypad);
  { // Rumble
    // Checking for basic capability
    REQUIRE(SDL_GameControllerHasRumble(gc));

    auto rumble_data = std::make_shared<std::pair<int, int>>();
    joypad.set_on_rumble([rumble_data](int low_freq, int high_freq) {
      rumble_data->first = low_freq;
      rumble_data->second = high_freq;
    });

    // When debugging this, bear in mind that SDL will send max duration here
    // https://github.com/libsdl-org/SDL/blob/da8fc70a83cf6b76d5ea75c39928a7961bd163d3/src/joystick/linux/SDL_sysjoystick.c#L1628
    SDL_GameControllerRumble(gc, 100, 200, 100);
    std::this_thread::sleep_for(30ms); // wait for the effect to be picked up
    REQUIRE(rumble_data->first == 200);
    REQUIRE(rumble_data->second == 100);
  }

  SDL_TEST_BUTTON(Joypad::MISC_FLAG, SDL_CONTROLLER_BUTTON_MISC1);

  { // Sticks
    REQUIRE(SDL_GameControllerHasAxis(gc, SDL_CONTROLLER_AXIS_LEFTX));
    REQUIRE(SDL_GameControllerHasAxis(gc, SDL_CONTROLLER_AXIS_LEFTY));
    REQUIRE(SDL_GameControllerHasAxis(gc, SDL_CONTROLLER_AXIS_RIGHTX));
    REQUIRE(SDL_GameControllerHasAxis(gc, SDL_CONTROLLER_AXIS_RIGHTY));
    REQUIRE(SDL_GameControllerHasAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERLEFT));
    REQUIRE(SDL_GameControllerHasAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERRIGHT));

    joypad.set_stick(Joypad::LS, 1000, 2000);
    flush_sdl_events();
    REQUIRE(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTX) == 1000);
    REQUIRE(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTY) == -2000);

    joypad.set_stick(Joypad::RS, 1000, 2000);
    flush_sdl_events();
    REQUIRE(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_RIGHTX) == 1000);
    REQUIRE(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_RIGHTY) == -2000);

    // Nintendo ONLY: triggers are buttons, so it can only be MAX or 0
    joypad.set_triggers(10, 20);
    flush_sdl_events();
    REQUIRE(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERLEFT) == 32767);
    REQUIRE(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) == 32767);

    joypad.set_triggers(0, 0);
    flush_sdl_events();
    REQUIRE(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERLEFT) == 0);
    REQUIRE(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) == 0);
  }

  SDL_GameControllerClose(gc);
}