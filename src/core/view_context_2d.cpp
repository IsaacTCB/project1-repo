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
    auto [local_x, local_y] = screen_to_view(x, y);
    return view_to_world(local_x, local_y);
}

std::tuple<float, float> ViewContext2D::world_to_screen(float x, float y) const {
    auto [local_x, local_y] = world_to_view(x, y);
    return view_to_screen(local_x, local_y);
}

std::tuple<float, float> ViewContext2D::screen_to_view(float x, float y) const {
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

    return {local_x, local_y};
}

std::tuple<float, float> ViewContext2D::view_to_screen(float x, float y) const {
    /* apply center offset */
    if (view_camera->anchor == Camera2D::AnchorMode::CENTER) {
        x += view_width * 0.5f;
        y += view_height * 0.5f;
    }

    /* convert view coords into screen coords */
    float screen_x = x * (window_viewport.width / view_width);
    float screen_y = y * (window_viewport.height / view_height);
    screen_x += window_viewport.x;
    screen_y += window_viewport.y;

    return {screen_x, screen_y};
}

std::tuple<float, float> ViewContext2D::world_to_view(float x, float y) const {
    /* apply camera translate */
    x -= view_camera->x;
    y -= view_camera->y;

    /* transform around rotation */
    if (angle_is_rotated(view_camera->rotation)) {
        /* TODO: cache sine and cosine directly within the camera object.
         * This should happen only if rotation has changed and is dirty. */
        float sin = std::sinf(view_camera->rotation);
        float cos = std::cosf(view_camera->rotation);

        float temp_x = x;
        float temp_y = y;
        x = (temp_x * cos) + (temp_y * sin);
        y = (-temp_x * sin) + (temp_y * cos);
    }

    x -= view_camera->offset_x;
    y -= view_camera->offset_y;

    /* offset and zoom (scale) */
    float zoom_factor = std::max(view_camera->zoom, EPSILON_APPROX);
    x *= zoom_factor;
    y *= zoom_factor;

    return {x, y};
}

std::tuple<float, float> ViewContext2D::view_to_world(float x, float y) const {
    /* offset and zoom (scale) */
    float zoom_factor = 1.0f / std::max(view_camera->zoom, EPSILON_APPROX);
    x *= zoom_factor;
    y *= zoom_factor;

    float world_x = view_camera->offset_x + x;
    float world_y = view_camera->offset_y + y;

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
