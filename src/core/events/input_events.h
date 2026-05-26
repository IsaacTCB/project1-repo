#pragma once

#include <string_view>
#include <variant>

#include "core/input/gamepad.h"
#include "core/input/keyboard.h"
#include "core/input/mouse.h"
#include "core/math/vec2.h"

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
    Vec2 wheel_motion;
    bool flipped;
};

struct EventInputMouseMotion {
    Vec2 screen_position;
    Vec2 relative;
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
