#pragma once

#include "core/math/vec2.h"
#include "runtime/asset_service.h"

struct Sprite2D {
    TextureRef texture;
    Vec2 uv_min, uv_max;
    Vec2 anchor; /* normalized coords, where 0.0 = left & 1.0 = right */
    Vec2 size;   /* in pixel size */
};
