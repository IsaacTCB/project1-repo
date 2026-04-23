#pragma once

#include <cstdint>
#include <variant>

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
    uint32_t button;
    bool pressed;
};

struct EventInputMouseMotion {
    float screen_x;
    float screen_y;
    float rel_x;
    float rel_y;
};

using EventInput = std::variant<
    EventInputKey,
    EventInputMouseButton,
    EventInputMouseMotion>;
