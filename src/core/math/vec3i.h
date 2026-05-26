#pragma once

#include <format>

class Vec2i;
class Vec3;

class Vec3i {
public:
    int x;
    int y;
    int z;

public:
    Vec3i()
        : x(0), y(0), z{0} {}

    explicit Vec3i(const Vec2i &vec, int z);
    explicit Vec3i(const Vec3 &vec);

    constexpr Vec3i(int x, int y, int z)
        : x(x), y(y), z(z) {}

    [[nodiscard]] int length_squared() const;
    [[nodiscard]] float length() const;
    [[nodiscard]] Vec3i cross(const Vec3i &other) const;
    [[nodiscard]] Vec3i abs() const;

public:
    /* compound assignments */

    constexpr Vec3i &operator+=(const Vec3i &rhs) {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    constexpr Vec3i &operator-=(const Vec3i &rhs) {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    constexpr Vec3i &operator*=(const Vec3i &rhs) {
        x *= rhs.x;
        y *= rhs.y;
        z *= rhs.z;
        return *this;
    }

    constexpr Vec3i &operator/=(const Vec3i &rhs) {
        x /= rhs.x;
        y /= rhs.y;
        z /= rhs.z;
        return *this;
    }

public:
    /* scalar assignments */

    constexpr Vec3i &operator*=(float scalar) {
        x = static_cast<int>((float)x * scalar);
        y = static_cast<int>((float)y * scalar);
        z = static_cast<int>((float)z * scalar);
        return *this;
    }

    constexpr Vec3i &operator/=(float scalar) {
        x = static_cast<int>((float)x / scalar);
        y = static_cast<int>((float)y / scalar);
        z = static_cast<int>((float)z / scalar);
        return *this;
    }

    constexpr Vec3i &operator*=(int scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    constexpr Vec3i &operator/=(int scalar) {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }

public:
    /* unary operators */

    constexpr Vec3i operator+() const {
        return *this;
    }

    constexpr Vec3i operator-() const {
        return {-x, -y, -z};
    }

public:
    /* vector with vector operators */

    constexpr friend Vec3i operator+(Vec3i lhs, const Vec3i &rhs) {
        lhs += rhs;
        return lhs;
    };

    constexpr friend Vec3i operator-(Vec3i lhs, const Vec3i &rhs) {
        lhs -= rhs;
        return lhs;
    };

    constexpr friend Vec3i operator*(Vec3i lhs, const Vec3i &rhs) {
        lhs *= rhs;
        return lhs;
    };

    constexpr friend Vec3i operator/(Vec3i lhs, const Vec3i &rhs) {
        lhs /= rhs;
        return lhs;
    };

    constexpr bool operator==(const Vec3i &other) const {
        return (x == other.x && y == other.y && z == other.z);
    }

    constexpr bool operator!=(const Vec3i &other) const {
        return !(*this == other);
    }

public:
    /* vector with scalar operators */

    constexpr friend Vec3i operator*(Vec3i lhs, float scalar) {
        lhs *= scalar;
        return lhs;
    };

    constexpr friend Vec3i operator*(float scalar, Vec3i lhs) {
        lhs *= scalar;
        return lhs;
    };

    constexpr friend Vec3i operator/(Vec3i lhs, float scalar) {
        lhs /= scalar;
        return lhs;
    };

    constexpr friend Vec3i operator*(Vec3i lhs, int scalar) {
        lhs *= scalar;
        return lhs;
    };

    constexpr friend Vec3i operator*(int scalar, Vec3i lhs) {
        lhs *= scalar;
        return lhs;
    };

    constexpr friend Vec3i operator/(Vec3i lhs, int scalar) {
        lhs /= scalar;
        return lhs;
    };
};

template <>
struct std::formatter<Vec3i> : std::formatter<std::string_view> {
    auto format(const Vec3i &vec, format_context &ctx) const {
        std::string s = std::format("({}, {}, {})", vec.x, vec.y, vec.z);
        return formatter<std::string_view>::format(s, ctx);
    }
};
