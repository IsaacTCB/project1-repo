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
        Vec2 tl; // top-left coord
        Vec2 br; // bottom-right coord

    public:
        [[nodiscard]]
        Vec2 size() const {
            return br - tl;
        }

        [[nodiscard]]
        float width() const {
            return br.x - tl.x;
        }

        [[nodiscard]]
        float height() const {
            return br.y - tl.y;
        }

        [[nodiscard]]
        float diagonal() const {
            return tl.distance(br);
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
