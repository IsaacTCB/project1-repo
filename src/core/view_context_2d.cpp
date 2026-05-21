#include "view_context_2d.h"
#include "core/camera.h"

#include <algorithm>
#include <array>
#include <cmath>

constexpr float EPSILON_APPROX = 0.001f;

inline static bool angle_is_rotated(float angle) {
    return (angle < -EPSILON_APPROX ||
            angle > EPSILON_APPROX);
}

std::tuple<float, float> ViewContext2D::screen_to_world(float x, float y) const {
    float local_x = x - window_viewport.x;
    float local_y = y - window_viewport.y;

    /* convert screen coords into view coords */
    local_x *= view_width / window_viewport.width;
    local_y *= view_height / window_viewport.height;

    /* apply center offset */
    if (view_camera->anchor == Camera2D::AnchorMode::CENTER) {
        local_x -= view_width * 0.5f;
        local_y -= view_height * 0.5f;
    }

    /* offset and zoom (scale) */
    float zoom_factor = 1.0f / std::max(view_camera->zoom, EPSILON_APPROX);
    float world_x = view_camera->offset_x +
                    (local_x * zoom_factor);
    float world_y = view_camera->offset_y +
                    (local_y * zoom_factor);

    /* transform around rotation */
    if (angle_is_rotated(view_camera->rotation)) {
        /* TODO: cache sine and cosine directly within the camera object.
         * This should happen only if rotation has changed and is dirty. */
        float sin = std::sinf(view_camera->rotation);
        float cos = std::cosf(view_camera->rotation);

        float temp_x = world_x;
        float temp_y = world_y;
        world_x = (temp_x * cos) - (temp_y * sin);
        world_y = (temp_x * sin) + (temp_y * cos);
    }

    /* apply camera translate */
    world_x += view_camera->x;
    world_y += view_camera->y;
    return {world_x, world_y};
}

ViewContext2D::BoundRect ViewContext2D::get_bounds_aabb() const {
    float zoom_factor = 1.0f / std::max(view_camera->zoom, EPSILON_APPROX);
    float width = view_width * zoom_factor;
    float height = view_height * zoom_factor;

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

    left += view_camera->offset_x;
    right += view_camera->offset_x;
    top += view_camera->offset_y;
    bottom += view_camera->offset_y;

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

    bound.left += view_camera->x;
    bound.right += view_camera->x;
    bound.top += view_camera->y;
    bound.bottom += view_camera->y;
    return bound;
}
