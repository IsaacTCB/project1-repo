#pragma once

#include "../utils/inline.h"

constexpr float EPSILON = 0.00001f;

FORCE_INLINE bool is_zero_approx(float value) {
    return (value > -EPSILON && value < EPSILON);
}
