#pragma once

#include <cstddef>
#include <cstdint>

namespace Input::Gamepad {

/* Adapted from SDL3:
 * https://github.com/libsdl-org/SDL/blob/main/include/SDL3/SDL_gamepad.h
 */
enum class Buttons : uint8_t {
    /* Face buttons */
    SOUTH,
    EAST,
    WEST,
    NORTH,

    /* Middle buttons */
    BACK,
    GUIDE,
    START,

    /* Face buttons */
    LEFT_STICK,
    RIGHT_STICK,
    LEFT_SHOULDER,
    RIGHT_SHOULDER,

    /* D-pads */
    DPAD_UP,
    DPAD_DOWN,
    DPAD_LEFT,
    DPAD_RIGHT,

    /* Additional buttons */
    RIGHT_PADDLE1, /* Upper or primary paddle, under your right hand (e.g. Xbox Elite paddle P1, DualSense Edge RB button, Right Joy-Con SR button) */
    LEFT_PADDLE1,  /* Upper or primary paddle, under your left hand (e.g. Xbox Elite paddle P3, DualSense Edge LB button, Left Joy-Con SL button) */
    RIGHT_PADDLE2, /* Lower or secondary paddle, under your right hand (e.g. Xbox Elite paddle P2, DualSense Edge right Fn button, Right Joy-Con SL button) */
    LEFT_PADDLE2,  /* Lower or secondary paddle, under your left hand (e.g. Xbox Elite paddle P4, DualSense Edge left Fn button, Left Joy-Con SR button) */
    TOUCHPAD,      /* PS4/PS5 touchpad button */

    /* Special buttons */
    MISC1, /* (e.g. Xbox Series X share button, PS5 microphone button, Nintendo Switch Pro capture button, Amazon Luna microphone button, Google Stadia capture button) */
    MISC2,
    MISC3, /* (e.g. Nintendo GameCube left trigger click) */
    MISC4, /* (e.g. Nintendo GameCube right trigger click) */
    MISC5,
    MISC6,

    COUNT,
};

enum class Axis : uint8_t {
    LEFT_HORIZONTAL,
    LEFT_VERTICAL,

    RIGHT_HORIZONTAL,
    RIGHT_VERTICAL,

    TRIGGER_LEFT,
    TRIGGER_RIGHT,

    COUNT,
};

using DeviceID = std::uint8_t;
constexpr auto BUTTON_COUNT = static_cast<size_t>(::Input::Gamepad::Buttons::COUNT);
constexpr auto AXIS_COUNT = static_cast<size_t>(::Input::Gamepad::Axis::COUNT);
constexpr int GAMEPAD_MAX = 16; /* Max number of gamepads supported at once. */

}; // namespace Input::Gamepad
