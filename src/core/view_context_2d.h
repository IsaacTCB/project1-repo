#pragma once

#include <tuple>

#include "core/camera.h"
#include "core/viewport.h"

/**
 * @class ViewContext2D
 * @brief Represents a viewport context. Contains
 * camera, view size and viewport position.
 *
 * TODO: Add more conversion functions (world_to_screen, view_coord_to_world, etc.)
 */
class ViewContext2D {
public:
    class BoundRect {
    public:
        float left;
        float right;
        float top;
        float bottom;

    public:
        [[nodiscard]]
        float width() const {
            return right - left;
        }

        [[nodiscard]]
        float height() const {
            return bottom - top;
        }
    };

public:
    Camera2D *view_camera;
    float view_width;
    float view_height;
    Viewport window_viewport;

public:
    [[nodiscard]]
    std::tuple<float, float> screen_to_world(float x, float y) const;

    [[nodiscard]]
    BoundRect get_bounds_aabb() const;
};
