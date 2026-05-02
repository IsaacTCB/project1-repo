#pragma once

#include <string_view>
#include <variant>

#include "core/keys/gamepad.h"
#include "core/keys/scancodes.h"

/* Keyboard events */

struct EventInputKey {
    Key::Scancode key;
    Key::Scancode scancode;
    bool pressed;
    bool repeat;
};

/* Mouse events */

struct EventInputMouseButton {
    Key::MouseButtons button;
    bool pressed;
};

struct EventInputMouseMotion {
    float screen_x;
    float screen_y;
    float rel_x;
    float rel_y;
};

/* Gamepad events */

struct EventInputGamepadConnected {
    Gamepad::DeviceID pad;
    std::string_view name;
};

struct EventInputGamepadDisconnected {
    Gamepad::DeviceID pad;
};

struct EventInputGamepadButton {
    Gamepad::DeviceID pad;
    Gamepad::Buttons button;
    bool pressed;
};

struct EventInputGamepadAxisMotion {
    Gamepad::DeviceID pad;
    Gamepad::Axis axis;
    float value;
};

using EventInput = std::variant<
    EventInputKey,
    EventInputMouseButton,
    EventInputMouseMotion,
    EventInputGamepadConnected,
    EventInputGamepadDisconnected,
    EventInputGamepadButton,
    EventInputGamepadAxisMotion>;
