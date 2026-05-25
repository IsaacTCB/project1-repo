#include "vec2i.h"

#include <cmath>

int Vec2i::length_squared() const {
    return (x * x) + (y * y);
}

float Vec2i::length() const {
    auto xx = static_cast<float>(x);
    auto yy = static_cast<float>(y);
    return std::sqrt((xx * xx) + (yy * yy));
}

Vec2i Vec2i::abs() const {
    return {
        (x > 0) ? x : -x,
        (y > 0) ? y : -y,
    };
}
