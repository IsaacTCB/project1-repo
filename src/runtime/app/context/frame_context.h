#pragma once

#include "core/math/color.h"
#include "renderer/api/render_queue.h"

struct FrameContext {
    /**
     * @brief Elapsed time between last and current frames.
     */
    float delta;
};

struct UpdateContext {
    const FrameContext &frame;
};

struct RenderContext {
    const FrameContext &frame;

    /* used in rendering */
    RenderQueue &queue;
    Color clear_color;
};
