#pragma once

#include <cstdint>
#include <variant>

/* Keyboard events */

struct EventInputKey {
    uint32_t key;
    uint32_t scancode;
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
