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
    /* get projection bounds */
    Vec2 tl = -camera_get_anchor_offset(*this); // top-left coords
    Vec2 br = tl + view_size;                   // bottom-right coords

    /* apply zoom */
    float zoom_factor = std::max(view_camera->zoom, EPSILON);
    tl /= zoom_factor;
    br /= zoom_factor;

    /* apply offset */
    tl += view_camera->offset;
    br += view_camera->offset;

    /* rotate bounds, if needed */
    if (angle_is_rotated(view_camera->rotation)) {
        std::array<Vec2, 4> corners{{
            {tl.x, tl.y}, // top-left
            {br.x, tl.y}, // top-right
            {br.x, br.y}, // bottom-right
            {tl.x, br.y}, // bottom-left
        }};

        /* we'll recalculate these */
        tl.clear();
        br.clear();

        /* rotate corners */
        float sin = std::sinf(view_camera->rotation);
        float cos = std::cosf(view_camera->rotation);
        for (const auto &corner : corners) {
            Vec2 rotated{
                (corner.x * cos) - (corner.y * sin),
                (corner.x * sin) + (corner.y * cos),
            };

            tl = tl.min(rotated);
            br = br.max(rotated);
        }
    }

    /* apply camera translation */
    tl += view_camera->position;
    br += view_camera->position;

    return {
        .tl = tl,
        .br = br,
    };
}

ViewContext2D::BoundRect ViewContext2D::get_ortho() const {
    /* init values */
    float left = 0.0f;
    float right = view_size.x;
    float top = 0.0f;
    float bottom = view_size.y;

    /* apply anchor offset */
    auto anchor = camera_get_anchor_offset(*this);
    left -= anchor.x;
    right -= anchor.x;
    top -= anchor.y;
    bottom -= anchor.y;

    /* zoom in */
    float zoom_factor = std::max(view_camera->zoom, EPSILON);
    left /= zoom_factor;
    right /= zoom_factor;
    top /= zoom_factor;
    bottom /= zoom_factor;

    return {
        .tl = Vec2{left, top},
        .br = Vec2{right, bottom},
    };
}

inline static bool angle_is_rotated(float angle, float eps) {
    /* HACK: checking if non-zero is not ideal bc it won't detect
     * angles near 360deg such as (2 * PI - 0.00001) */
    return (std::fabs(angle) > eps);
}

inline static Vec2 camera_get_anchor_offset(const ViewContext2D &ctx) {
    if (ctx.view_camera->anchor == Camera2D::AnchorMode::CENTER) {
        return ctx.view_size * 0.5f;
    }
    return Vec2{};
}
