#pragma once

#include <cstdint>

#include "core/math/color.h"
#include "core/view_context_2d.h"
#include "core/view_layout.h"

#include "runtime/renderer/render_queue_builder.h"

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
     * @brief Non-owning pointer of the ViewLayout object
     * for this frame.
     */
    const ViewLayout *view_layout;
};

struct UpdateContext {
    const FrameContext &frame;
    bool new_second;
};

struct RenderContext {
    FrameContext &frame;

    /* final ViewContext */
    ViewContext2D *view_context;

    /* used in rendering */
    RenderQueueBuilder queue;
    Color clear_color;
};
