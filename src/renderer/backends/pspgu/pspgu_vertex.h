#pragma once

#include <cstdint>
#include <psptypes.h>

struct PSPGuVertex {
    float u, v;
    uint32_t color;
    ScePspFVector3 position;
};

// clang-format off
alignas(16) inline const uint8_t g_quad_indices[6] = {
    0, 1, 2,
    2, 3, 0,
};
// clang-format on
