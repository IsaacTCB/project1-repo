#pragma once

#include <cstdint>

class ICamera {};

class Camera2D : public ICamera {
public:
    enum class AnchorMode : std::uint8_t {
        TOP_LEFT,
        CENTER,
    };

public:
    /* TODO: make vec2 structs and then replace these */
    float x = 0.0f;
    float y = 0.0f;
    float offset_x = 0.0f;
    float offset_y = 0.0f;

    float rotation = 0.0f; /* in radians */
    float zoom = 1.0f;
    AnchorMode anchor = AnchorMode::TOP_LEFT;
};
