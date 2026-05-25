#pragma once

class Vec3 {
public:
    float x;
    float y;
    float z;

public:
    Vec3()
        : x(0.0f), y(0.0f), z{0.0f} {}

    constexpr Vec3(float x, float y, float z)
        : x(x), y(y), z(z) {}

    [[nodiscard]] float length_squared() const;
    [[nodiscard]] float length() const;
    [[nodiscard]] float dot(const Vec3 &other) const;
    [[nodiscard]] Vec3 cross(const Vec3 &other) const;
    [[nodiscard]] float distance(const Vec3 &other) const;
    [[nodiscard]] float distance_squared(const Vec3 &other) const;
    [[nodiscard]] Vec3 normalized() const;
    [[nodiscard]] Vec3 abs() const;

    void normalize();

public:
    /* compound assignments */

    constexpr Vec3 &operator+=(const Vec3 &rhs) {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    constexpr Vec3 &operator-=(const Vec3 &rhs) {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    constexpr Vec3 &operator*=(const Vec3 &rhs) {
        x *= rhs.x;
        y *= rhs.y;
        z *= rhs.z;
        return *this;
    }

    constexpr Vec3 &operator/=(const Vec3 &rhs) {
        x /= rhs.x;
        y /= rhs.y;
        z /= rhs.z;
        return *this;
    }

public:
    /* scalar assignments */

    constexpr Vec3 &operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    constexpr Vec3 &operator/=(float scalar) {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }

public:
    /* unary operators */

    constexpr Vec3 operator+() const {
        return *this;
    }

    constexpr Vec3 operator-() const {
        return {-x, -y, -z};
    }

public:
    /* vector with vector operators */

    constexpr friend Vec3 operator+(Vec3 lhs, const Vec3 &rhs) {
        lhs += rhs;
        return lhs;
    };

    constexpr friend Vec3 operator-(Vec3 lhs, const Vec3 &rhs) {
        lhs -= rhs;
        return lhs;
    };

    constexpr friend Vec3 operator*(Vec3 lhs, const Vec3 &rhs) {
        lhs *= rhs;
        return lhs;
    };

    constexpr friend Vec3 operator/(Vec3 lhs, const Vec3 &rhs) {
        lhs /= rhs;
        return lhs;
    };

    constexpr bool operator==(const Vec3 &other) const {
        return (x == other.x && y == other.y && z == other.z);
    }

    constexpr bool operator!=(const Vec3 &other) const {
        return !(*this == other);
    }

public:
    /* vector with scalar operators */

    constexpr friend Vec3 operator*(Vec3 lhs, float scalar) {
        lhs *= scalar;
        return lhs;
    };

    constexpr friend Vec3 operator*(float scalar, Vec3 lhs) {
        lhs *= scalar;
        return lhs;
    };

    constexpr friend Vec3 operator/(Vec3 lhs, float scalar) {
        lhs /= scalar;
        return lhs;
    };
};
