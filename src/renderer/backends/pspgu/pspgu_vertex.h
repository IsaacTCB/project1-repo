#pragma once

#include <cstdint>
#include <psptypes.h>

struct PSPGuVertex {
    float u, v;
    uint32_t color;
    ScePspFVector3 position;
};
