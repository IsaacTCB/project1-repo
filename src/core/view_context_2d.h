#pragma once

#include <tuple>

#include "core/camera.h"
#include "core/viewport.h"

/**
 * @class ViewContext2D
 * @brief Represents a viewport context. Contains
 * camera, view size and viewport position.
 *
 * TODO: Add more conversion functions (world_to_screen, get_bounds, etc.)
 */
class ViewContext2D {
public:
    Camera2D *view_camera;
    float view_width;
    float view_height;
    Viewport window_viewport;

public:
    [[nodiscard]]
    std::tuple<float, float> screen_to_world(float x, float y) const;
};
