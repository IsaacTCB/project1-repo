#pragma once

#include <cstdint>
#include <variant>

struct EventWindowResized {
    uint32_t width, height;
};

struct EventWindowMaximized {};
struct EventWindowMinimized {};

using EventWindow = std::variant<
    EventWindowResized,
    EventWindowMaximized,
    EventWindowMinimized>;
