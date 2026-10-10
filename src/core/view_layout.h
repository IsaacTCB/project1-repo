#pragma once

#include "core/math/vec2.h"
#include "core/math/vec2i.h"
#include "core/viewport.h"

struct ViewLayout {
    Vec2 logical_size;

    Vec2i render_target_size;
    Viewport render_viewport;

    Viewport presentation_rect;

    Vec2 scale;
};
