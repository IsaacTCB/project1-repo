#include "vec3.h"
#include "vec3i.h"
#include "vec2.h"

#include "base.h"

#include <cmath>

Vec3::Vec3(const Vec2 &vec, float z)
    : x(vec.x), y(vec.y), z(z) {}

Vec3::Vec3(const Vec3i &vec)
    : x((float)vec.x), y((float)vec.y), z((float)vec.z) {}

float Vec3::length_squared() const {
    return (x * x) + (y * y) + (z * z);
}

float Vec3::length() const {
    return std::sqrt((x * x) + (y * y) + (z * z));
}

float Vec3::dot(const Vec3 &other) const {
    return (x * other.x) + (y * other.y) + (z * other.z);
}

Vec3 Vec3::cross(const Vec3 &other) const {
    return {
        (y * other.z) - (z * other.y),
        (z * other.x) - (x * other.z),
        (x * other.y) - (y * other.x),
    };
}
float Vec3::distance(const Vec3 &other) const {
    return (other - *this).length();
}
float Vec3::distance_squared(const Vec3 &other) const {
    return (other - *this).length_squared();
}

Vec3 Vec3::normalized() const {
    Vec3 v{*this};
    v.normalize();
    return v;
}

Vec3 Vec3::limited_length(float length) const {
    Vec3 v{*this};
    v.limit_length(length);
    return v;
}

Vec3 Vec3::abs() const {
    return {
        std::fabs(x),
        std::fabs(y),
        std::fabs(z),
    };
}

void Vec3::normalize() {
    float len = length();
    if (len > EPSILON) {
        x /= len;
        y /= len;
        z /= len;
        return;
    }
    x = 0.0f;
    y = 0.0f;
    z = 0.0f;
}

void Vec3::limit_length(float length) {
    if (length <= EPSILON) {
        *this = {};
        return;
    }

    float len = this->length();
    if (len > length) {
        float factor = (length / len);
        x *= factor;
        y *= factor;
        z *= factor;
    }
}