#pragma once

#include "core/math/color.h"
#include "core/math/vec2.h"
#include "core/math/vec2i.h"

struct TransformComponent {
    Vec2 position;
};

struct VelocityComponent {
    Vec2 velocity;
};

struct InputComponent {};

struct CanvasRectComponent {
    Vec2i size;
    Color color;
};
