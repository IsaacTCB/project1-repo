#pragma once

#include "core/camera.h"
#include "core/viewport.h"

class ViewContext2D {
public:
    Camera2D *view_camera;
    float view_width;
    float view_height;
    Viewport window_viewport;

public:
    [[nodiscard]]
    static ViewContext2D build(Camera2D &camera, Viewport viewport) noexcept {
        return {
            .view_camera = &camera,
            .view_width = viewport.width,
            .view_height = viewport.height,
            .window_viewport = viewport,
        };
    }
};
