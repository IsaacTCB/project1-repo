#include "vec3i.h"
#include "vec2i.h"
#include "vec3.h"

#include <cmath>

Vec3i::Vec3i(const Vec2i &vec, int z)
    : x(vec.x), y(vec.y), z(z) {}

Vec3i::Vec3i(const Vec3 &vec)
    : x((int)vec.x), y((int)vec.y), z((int)vec.z) {}

int Vec3i::length_squared() const {
    return (x * x) + (y * y) + (z * z);
}

float Vec3i::length() const {
    auto xx = static_cast<float>(x);
    auto yy = static_cast<float>(y);
    auto zz = static_cast<float>(z);
    return std::sqrt((xx * xx) + (yy * yy) + (zz * zz));
}

Vec3i Vec3i::cross(const Vec3i &other) const {
    return {
        (y * other.z) - (z * other.y),
        (z * other.x) - (x * other.z),
        (x * other.y) - (y * other.x),
    };
}

Vec3i Vec3i::abs() const {
    return {
        (x > 0) ? x : -x,
        (y > 0) ? y : -y,
        (z > 0) ? z : -z,
    };
}

Vec3i Vec3i::min(const Vec3i &rhs) const {
    return {
        std::min(x, rhs.x),
        std::min(y, rhs.y),
        std::min(z, rhs.z),
    };
}

Vec3i Vec3i::max(const Vec3i &rhs) const {
    return {
        std::max(x, rhs.x),
        std::max(y, rhs.y),
        std::max(z, rhs.z),
    };
}
