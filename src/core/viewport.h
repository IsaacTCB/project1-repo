#pragma once

#include "core/math/vec2i.h"

class Viewport {
public:
    Vec2i position;
    Vec2i size;

public:
    [[nodiscard]]
    float get_aspect_ratio() const {
        return static_cast<float>(size.x) / static_cast<float>(size.y);
    }
};
