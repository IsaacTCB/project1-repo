#pragma once

#include "core/camera.h"
#include "core/viewport.h"

struct RenderView {
    ICamera *camera;
    Viewport viewport;
};
