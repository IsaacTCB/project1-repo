#pragma once

#include "core/math/vec2.h"

#include <cstdint>

class ICamera {};

class Camera2D : public ICamera {
public:
    enum class AnchorMode : std::uint8_t {
        TOP_LEFT,
        CENTER,
    };

public:
    Vec2 position;
    Vec2 offset;

    float rotation = 0.0f; /* in radians */
    float zoom = 1.0f;
    AnchorMode anchor = AnchorMode::TOP_LEFT;
};
