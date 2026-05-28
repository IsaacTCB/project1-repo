#include "vec2.h"
#include "vec2i.h"

#include "base.h"

#include <cmath>

Vec2::Vec2(const Vec2i &vec)
    : x((float)vec.x), y((float)vec.y) {};

float Vec2::length_squared() const {
    return (x * x) + (y * y);
}

float Vec2::length() const {
    return std::sqrt((x * x) + (y * y));
}

float Vec2::dot(const Vec2 &other) const {
    return (x * other.x) + (y * other.y);
}

float Vec2::cross(const Vec2 &other) const {
    return (x * other.y) - (y * other.x);
}

float Vec2::distance(const Vec2 &other) const {
    return (other - *this).length();
}

float Vec2::distance_squared(const Vec2 &other) const {
    return (other - *this).length_squared();
}

Vec2 Vec2::normalized() const {
    Vec2 v{*this};
    v.normalize();
    return v;
}

Vec2 Vec2::limited_length(float length) const {
    Vec2 v{*this};
    v.limit_length(length);
    return v;
}

Vec2 Vec2::abs() const {
    return {
        std::fabs(x),
        std::fabs(y),
    };
}

void Vec2::normalize() {
    float len = length();
    if (len > EPSILON) {
        x /= len;
        y /= len;
        return;
    }
    x = 0.0f;
    y = 0.0f;
}

void Vec2::limit_length(float length) {
    if (length <= EPSILON) {
        clear();
        return;
    }

    float len = this->length();
    if (len >= length) {
        float factor = (length / len);
        x *= factor;
        y *= factor;
    }
}
