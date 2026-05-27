#include "view_context_2d.h"

#include "core/camera.h"
#include "core/math/base.h"
#include "core/math/vec2.h"

#include <algorithm>
#include <array>
#include <cmath>

inline static bool angle_is_rotated(float angle, float eps = EPSILON);
inline static Vec2 camera_get_anchor_offset(const ViewContext2D &ctx);

/* ================================ */
/*  Screen <--> World conversions   */
/* ================================ */

Vec2 ViewContext2D::screen_to_world(const Vec2 &screen_coord) const {
    auto view_coord = screen_to_view(screen_coord);
    return view_to_world(view_coord);
}

Vec2 ViewContext2D::world_to_screen(const Vec2 &world_coord) const {
    auto view_coord = world_to_view(world_coord);
    return view_to_screen(view_coord);
}

/* ================================ */
/*   Screen <--> View conversions   */
/* ================================ */

Vec2 ViewContext2D::screen_to_view(const Vec2 &screen_coord) const {
    Vec2 view{screen_coord};

    view -= window_viewport.position;
    view *= view_size / window_viewport.size;
    view -= camera_get_anchor_offset(*this);

    return {view.x, view.y};
}

Vec2 ViewContext2D::view_to_screen(const Vec2 &view_coord) const {
    Vec2 screen{view_coord};

    screen += camera_get_anchor_offset(*this);
    screen *= window_viewport.size / view_size;
    screen += window_viewport.position;

    return screen;
}

/* ================================ */
/*    View <--> World conversions   */
/* ================================ */

Vec2 ViewContext2D::view_to_world(const Vec2 &view_coord) const {
    Vec2 world{view_coord};

    /* apply zoom (scale) */
    float zoom_factor = std::max(view_camera->zoom, EPSILON);
    world /= zoom_factor;

    /* apply offset */
    world += view_camera->offset;

    /* transform around rotation clockwise */
    if (angle_is_rotated(view_camera->rotation)) {
        /* TODO: cache sine and cosine directly within the camera object.
         * This should happen only if rotation has changed and is dirty. */
        float sin = std::sinf(view_camera->rotation);
        float cos = std::cosf(view_camera->rotation);

        auto temp{world};
        world.x = (temp.x * cos) - (temp.y * sin);
        world.y = (temp.x * sin) + (temp.y * cos);
    }

    /* apply camera translate */
    world += view_camera->position;

    return world;
}

Vec2 ViewContext2D::world_to_view(const Vec2 &world_coord) const {
    Vec2 view{world_coord};

    /* revert camera translate */
    view -= view_camera->position;

    /* transform around rotation counter-clockwise */
    if (angle_is_rotated(view_camera->rotation)) {
        /* TODO: cache sine and cosine directly within the camera object.
         * This should happen only if rotation has changed and is dirty. */
        float sin = std::sinf(view_camera->rotation);
        float cos = std::cosf(view_camera->rotation);

        Vec2 temp{view};
        view.x = (temp.x * cos) + (temp.y * sin);
        view.y = (-temp.x * sin) + (temp.y * cos);
    }

    /* revert offset */
    view -= view_camera->offset;

    /* revert zoom (scale) */
    float zoom_factor = std::max(view_camera->zoom, EPSILON);
    view *= zoom_factor;

    return view;
}

/* =================== */
/*    Camera bounds    */
/* =================== */

ViewContext2D::BoundRect ViewContext2D::get_bounds_aabb() const {
    float zoom_factor = 1.0f / std::max(view_camera->zoom, EPSILON);
    float width = view_size.x * zoom_factor;
    float height = view_size.y * zoom_factor;

    /* get actual position */
    float left;
    float right;
    float top;
    float bottom;
    if (view_camera->anchor == Camera2D::AnchorMode::CENTER) {
        /* apply center offset */
        float width_half = width * 0.5f;
        float height_half = height * 0.5f;
        left = -width_half;
        right = width_half;
        top = -height_half;
        bottom = height_half;
    } else {
        /* get top right bounds */
        left = 0.0f;
        right = width;
        top = 0.0f;
        bottom = height;
    }

    left += view_camera->offset.x;
    right += view_camera->offset.x;
    top += view_camera->offset.y;
    bottom += view_camera->offset.y;

    BoundRect bound{};
    if (angle_is_rotated(view_camera->rotation)) {
        float sin = std::sinf(view_camera->rotation);
        float cos = std::cosf(view_camera->rotation);

        std::array<std::array<float, 2>, 4> points{{
            {{left, top}},
            {{right, top}},
            {{right, bottom}},
            {{left, bottom}},
        }};

        for (auto &vec2 : points) {
            float x = vec2[0];
            float y = vec2[1];

            float rx = (x * cos) - (y * sin);
            float ry = (x * sin) + (y * cos);

            bound.left = std::min(bound.left, rx);
            bound.top = std::min(bound.top, ry);
            bound.right = std::max(bound.right, rx);
            bound.bottom = std::max(bound.bottom, ry);
        }
    } else {
        bound.left = left;
        bound.right = right;
        bound.top = top;
        bound.bottom = bottom;
    }

    bound.left += view_camera->position.x;
    bound.right += view_camera->position.x;
    bound.top += view_camera->position.y;
    bound.bottom += view_camera->position.y;
    return bound;
}

inline static bool angle_is_rotated(float angle, float eps) {
    return (angle < -eps ||
            angle > eps);
}

inline static Vec2 camera_get_anchor_offset(const ViewContext2D &ctx) {
    if (ctx.view_camera->anchor == Camera2D::AnchorMode::CENTER) {
        return ctx.view_size * 0.5f;
    }
    return Vec2{};
}
