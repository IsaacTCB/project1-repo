#pragma once

#include <cstddef>

#include "core/math/vec2.h"

inline constexpr size_t GLYPH_ATLAS_WIDTH = 512;
inline constexpr size_t GLYPH_ATLAS_HEIGHT = 512;

struct GlyphQuad {
    Vec2 uv_min, uv_max;
    Vec2 position, size;
};
