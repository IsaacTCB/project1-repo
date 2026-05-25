#pragma once

class Vec2i {
public:
    int x;
    int y;

public:
    Vec2i()
        : x(0), y(0) {}

    constexpr Vec2i(int x, int y)
        : x(x), y(y) {}

    [[nodiscard]] int length_squared() const;
    [[nodiscard]] float length() const;
    [[nodiscard]] Vec2i abs() const;

public:
    /* compound assignments */

    constexpr Vec2i &operator+=(const Vec2i &rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    constexpr Vec2i &operator-=(const Vec2i &rhs) {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    constexpr Vec2i &operator*=(const Vec2i &rhs) {
        x *= rhs.x;
        y *= rhs.y;
        return *this;
    }

    constexpr Vec2i &operator/=(const Vec2i &rhs) {
        x /= rhs.x;
        y /= rhs.y;
        return *this;
    }

public:
    /* scalar assignments */

    constexpr Vec2i &operator*=(float scalar) {
        x = static_cast<int>((float)x * scalar);
        y = static_cast<int>((float)y * scalar);
        return *this;
    }

    constexpr Vec2i &operator/=(float scalar) {
        x = static_cast<int>((float)x / scalar);
        y = static_cast<int>((float)y / scalar);
        return *this;
    }

    constexpr Vec2i &operator*=(int scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    constexpr Vec2i &operator/=(int scalar) {
        x /= scalar;
        y /= scalar;
        return *this;
    }

public:
    /* unary operators */

    constexpr Vec2i operator+() const {
        return *this;
    }

    constexpr Vec2i operator-() const {
        return {-x, -y};
    }

public:
    /* vector with vector operators */

    constexpr friend Vec2i operator+(Vec2i lhs, const Vec2i &rhs) {
        lhs += rhs;
        return lhs;
    };

    constexpr friend Vec2i operator-(Vec2i lhs, const Vec2i &rhs) {
        lhs -= rhs;
        return lhs;
    };

    constexpr friend Vec2i operator*(Vec2i lhs, const Vec2i &rhs) {
        lhs *= rhs;
        return lhs;
    };

    constexpr friend Vec2i operator/(Vec2i lhs, const Vec2i &rhs) {
        lhs /= rhs;
        return lhs;
    };

    constexpr bool operator==(const Vec2i &other) const {
        return (x == other.x && y == other.y);
    }

    constexpr bool operator!=(const Vec2i &other) const {
        return !(*this == other);
    }

public:
    /* vector with scalar operators */

    constexpr friend Vec2i operator*(Vec2i lhs, float scalar) {
        lhs *= scalar;
        return lhs;
    };

    constexpr friend Vec2i operator*(float scalar, Vec2i lhs) {
        lhs *= scalar;
        return lhs;
    };

    constexpr friend Vec2i operator/(Vec2i lhs, float scalar) {
        lhs /= scalar;
        return lhs;
    };

    constexpr friend Vec2i operator*(Vec2i lhs, int scalar) {
        lhs *= scalar;
        return lhs;
    };

    constexpr friend Vec2i operator*(int scalar, Vec2i lhs) {
        lhs *= scalar;
        return lhs;
    };

    constexpr friend Vec2i operator/(Vec2i lhs, int scalar) {
        lhs /= scalar;
        return lhs;
    };
};
