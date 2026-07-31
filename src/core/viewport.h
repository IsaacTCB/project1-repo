#pragma once

#include "core/math/vec2.h"

class Viewport {
public:
    Vec2 position;
    Vec2 size;

public:
    [[nodiscard]]
    float get_aspect_ratio() const {
        return size.x / size.y;
    }
};
