#pragma once

#include <variant>

#include "core/math/vec2i.h"

/* TODO: add window index to all events */

struct EventWindowResized {
    Vec2i size;
};

struct EventWindowMoved {
    Vec2i position;
};

struct EventWindowMaximized {};
struct EventWindowMinimized {};

using EventWindow = std::variant<
    EventWindowResized,
    EventWindowMoved,
    EventWindowMaximized,
    EventWindowMinimized>;
