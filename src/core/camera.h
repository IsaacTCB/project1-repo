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
    float x;
    float y;
    float zoom;
    AnchorMode anchor;
};
