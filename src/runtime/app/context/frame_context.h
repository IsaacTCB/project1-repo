#pragma once

#include <cstdint>

#include "core/math/color.h"
#include "core/view_context_2d.h"
#include "core/viewport.h"
#include "renderer/api/render_queue.h"

struct FrameContext {
    /**
     * @brief Elapsed time between last and current frames.
     */
    float delta;

    /**
     * @brief How many frames have passed since last second?
     * Refreshes every second.
     */
    uint32_t fps;

    /**
     * @brief Current screen viewport for this frame
     */
    Viewport viewport;
};

struct UpdateContext {
    const FrameContext &frame;

    /* shared with RenderContext */
    ViewContext2D &view_context;

    bool new_second;
};

struct RenderContext {
    FrameContext &frame;

    /* final ViewContext */
    ViewContext2D &view_context;

    /* used in rendering */
    RenderQueue &queue;
    Color clear_color;
};
