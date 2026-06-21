#pragma once

#include <format>

class Vec2i;

class Vec2 {
public:
    float x;
    float y;

public:
    Vec2()
        : x(0.0f), y(0.0f) {}

    explicit Vec2(const Vec2i &vec);

    constexpr Vec2(float x, float y)
        : x(x), y(y) {}

    [[nodiscard]] float length_squared() const;
    [[nodiscard]] float length() const;
    [[nodiscard]] float dot(const Vec2 &other) const;
    [[nodiscard]] float cross(const Vec2 &other) const;
    [[nodiscard]] float distance(const Vec2 &other) const;
    [[nodiscard]] float distance_squared(const Vec2 &other) const;
    [[nodiscard]] Vec2 normalized() const;
    [[nodiscard]] Vec2 limited_length(float length) const;
    [[nodiscard]] Vec2 abs() const;
    [[nodiscard]] Vec2 min(const Vec2 &rhs) const;
    [[nodiscard]] Vec2 max(const Vec2 &rhs) const;

    void clear() { x = y = 0.0f; }
    void normalize();
    void limit_length(float length);

public:
    /* compound assignments */

    constexpr Vec2 &operator+=(const Vec2 &rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    constexpr Vec2 &operator-=(const Vec2 &rhs) {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    constexpr Vec2 &operator*=(const Vec2 &rhs) {
        x *= rhs.x;
        y *= rhs.y;
        return *this;
    }

    constexpr Vec2 &operator/=(const Vec2 &rhs) {
        x /= rhs.x;
        y /= rhs.y;
        return *this;
    }

public:
    /* scalar assignments */

    constexpr Vec2 &operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    constexpr Vec2 &operator/=(float scalar) {
        x /= scalar;
        y /= scalar;
        return *this;
    }

public:
    /* unary operators */

    constexpr Vec2 operator+() const {
        return *this;
    }

    constexpr Vec2 operator-() const {
        return {-x, -y};
    }

public:
    /* vector with vector operators */

    constexpr friend Vec2 operator+(Vec2 lhs, const Vec2 &rhs) {
        lhs += rhs;
        return lhs;
    };

    constexpr friend Vec2 operator-(Vec2 lhs, const Vec2 &rhs) {
        lhs -= rhs;
        return lhs;
    };

    constexpr friend Vec2 operator*(Vec2 lhs, const Vec2 &rhs) {
        lhs *= rhs;
        return lhs;
    };

    constexpr friend Vec2 operator/(Vec2 lhs, const Vec2 &rhs) {
        lhs /= rhs;
        return lhs;
    };

    constexpr bool operator==(const Vec2 &other) const {
        return (x == other.x && y == other.y);
    }

    constexpr bool operator!=(const Vec2 &other) const {
        return !(*this == other);
    }

public:
    /* vector with scalar operators */

    constexpr friend Vec2 operator*(Vec2 lhs, float scalar) {
        lhs *= scalar;
        return lhs;
    };

    constexpr friend Vec2 operator*(float scalar, Vec2 lhs) {
        lhs *= scalar;
        return lhs;
    };

    constexpr friend Vec2 operator/(Vec2 lhs, float scalar) {
        lhs /= scalar;
        return lhs;
    };
};

static_assert(sizeof(Vec2) == sizeof(float) * 2);

template <>
struct std::formatter<Vec2> : std::formatter<std::string_view> {
    auto format(const Vec2 &vec, format_context &ctx) const {
        std::string s = std::format("({}, {})", vec.x, vec.y);
        return formatter<std::string_view>::format(s, ctx);
    }
};
