#pragma once

#include <string_view>
#include <variant>

#include "core/input/gamepad.h"
#include "core/input/keyboard.h"
#include "core/input/mouse.h"

/* Keyboard events */

struct EventInputKey {
    Input::Key::Scancode key;
    Input::Key::Scancode scancode;
    bool pressed;
    bool repeat;
};

/* Mouse events */

struct EventInputMouseButton {
    Input::Mouse::Buttons button;
    bool pressed;
};

struct EventInputMouseWheel {
    float x;
    float y;
    bool flipped;
};

struct EventInputMouseMotion {
    float screen_x;
    float screen_y;
    float rel_x;
    float rel_y;
};

/* Gamepad events */

struct EventInputGamepadConnected {
    Input::Gamepad::DeviceID pad;
    std::string_view name;
};

struct EventInputGamepadDisconnected {
    Input::Gamepad::DeviceID pad;
};

struct EventInputGamepadButton {
    Input::Gamepad::DeviceID pad;
    Input::Gamepad::Buttons button;
    bool pressed;
};

struct EventInputGamepadAxisMotion {
    Input::Gamepad::DeviceID pad;
    Input::Gamepad::Axis axis;
    float value;
};

using EventInput = std::variant<
    EventInputKey,
    EventInputMouseButton,
    EventInputMouseMotion,
    EventInputMouseWheel,
    EventInputGamepadConnected,
    EventInputGamepadDisconnected,
    EventInputGamepadButton,
    EventInputGamepadAxisMotion>;
