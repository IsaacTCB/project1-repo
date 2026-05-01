#pragma once

#include <cstdint>

namespace Gamepad {

using Handle = std::int8_t;
constexpr int GAMEPAD_MAX = 16; /* Max number of gamepads supported at once. */

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

    /* Special buttons */
    MISC1,         /* (e.g. Xbox Series X share button, PS5 microphone button, Nintendo Switch Pro capture button, Amazon Luna microphone button, Google Stadia capture button) */
    RIGHT_PADDLE1, /* Upper or primary paddle, under your right hand (e.g. Xbox Elite paddle P1, DualSense Edge RB button, Right Joy-Con SR button) */
    LEFT_PADDLE1,  /* Upper or primary paddle, under your left hand (e.g. Xbox Elite paddle P3, DualSense Edge LB button, Left Joy-Con SL button) */
    RIGHT_PADDLE2, /* Lower or secondary paddle, under your right hand (e.g. Xbox Elite paddle P2, DualSense Edge right Fn button, Right Joy-Con SL button) */
    LEFT_PADDLE2,  /* Lower or secondary paddle, under your left hand (e.g. Xbox Elite paddle P4, DualSense Edge left Fn button, Left Joy-Con SR button) */
    TOUCHPAD,      /* PS4/PS5 touchpad button */

    /* Additional buttons */
    MISC2,
    MISC3, /* (e.g. Nintendo GameCube left trigger click) */
    MISC4, /* (e.g. Nintendo GameCube right trigger click) */
    MISC5,
    MISC6,

    COUNT,
};

}; // namespace Gamepad
