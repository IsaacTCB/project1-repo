#pragma once

#include <cstdint>

#include "core/math/color.h"
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
};

struct UpdateContext {
    const FrameContext &frame;
    bool new_second;
};

struct RenderContext {
    const FrameContext &frame;

    /* used in rendering */
    RenderQueue &queue;
    Color clear_color;
};
