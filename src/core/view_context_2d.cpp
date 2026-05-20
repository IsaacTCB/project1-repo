#include "view_context_2d.h"
#include "core/camera.h"
#include <cmath>

constexpr float EPSILON_APPROX = 0.001f;

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
                    (local_x / zoom_factor);
    float world_y = view_camera->offset_y +
                    (local_y * zoom_factor);

    /* transform around rotation */
    if (view_camera->rotation < -EPSILON_APPROX ||
        view_camera->rotation > EPSILON_APPROX) {
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
