#pragma once

#include <cstdint>
#include <variant>

struct EventWindowResized {
    uint32_t width, height;
};

struct EventWindowMoved {
    int x, y;
};

struct EventWindowMaximized {};
struct EventWindowMinimized {};

using EventWindow = std::variant<
    EventWindowResized,
    EventWindowMoved,
    EventWindowMaximized,
    EventWindowMinimized>;
