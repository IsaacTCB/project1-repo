#pragma once

#include "core/camera.h"
#include "core/math/vec2.h"
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
    Vec2 view_size;
    Viewport window_viewport;

public:
    [[nodiscard]]
    Vec2 screen_to_world(const Vec2 &screen_coord) const;

    [[nodiscard]]
    Vec2 world_to_screen(const Vec2 &world_coord) const;

    [[nodiscard]]
    Vec2 screen_to_view(const Vec2 &screen_coord) const;

    [[nodiscard]]
    Vec2 view_to_screen(const Vec2 &view_coord) const;

    [[nodiscard]]
    Vec2 view_to_world(const Vec2 &view_coord) const;

    [[nodiscard]]
    Vec2 world_to_view(const Vec2 &world_coord) const;

    [[nodiscard]]
    BoundRect get_bounds_aabb() const;
};
