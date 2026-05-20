#pragma once

#include "core/camera.h"
#include "core/viewport.h"

class ViewContext2D {
public:
    Camera2D *view_camera;
    float view_width;
    float view_height;
    Viewport window_viewport;
};
