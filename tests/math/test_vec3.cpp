#include "core/math/vec3.h"
#include "core/math/vec3i.h"

#include <gtest/gtest.h>

constexpr float EPSILON = 0.001f;

static inline void vec3_expect_near(const Vec3 &a, const Vec3 &b, float epsilon = EPSILON) {
    EXPECT_NEAR(a.x, b.x, epsilon);
    EXPECT_NEAR(a.y, b.y, epsilon);
    EXPECT_NEAR(a.z, b.z, epsilon);
}

/* Constructor tests */

TEST(Vec3ConstructorTest, DefaultConstructor) {
    Vec3 v;
    EXPECT_FLOAT_EQ(v.x, 0.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
    EXPECT_FLOAT_EQ(v.z, 0.0f);
}

TEST(Vec3ConstructorTest, ValueConstructor) {
    Vec3 v{3.0f, 4.0f, 2.0f};
    EXPECT_FLOAT_EQ(v.x, 3.0f);
    EXPECT_FLOAT_EQ(v.y, 4.0f);
    EXPECT_FLOAT_EQ(v.z, 2.0f);
}

TEST(Vec3ConstructorTest, IntConversionConstructor) {
    Vec3i a{4, 2, 4};
    Vec3 b{a};
    EXPECT_EQ(b.x, (float)a.x);
    EXPECT_EQ(b.y, (float)a.y);
    EXPECT_EQ(b.z, (float)a.z);
}

/* Arithmetic Operations */

TEST(Vec3ArithmeticTest, Addition) {
    Vec3 a{10.0f, 6.0f, 8.0f};
    Vec3 b{2.0f, -3.0f, 1.0f};
    vec3_expect_near((a + b), Vec3{12.0f, 3.0f, 9.0f});
}

TEST(Vec3ArithmeticTest, Subtraction) {
    Vec3 a{10.0f, 6.0f, 8.0f};
    Vec3 b{2.0f, -3.0f, 1.0f};
    vec3_expect_near((a - b), Vec3{8.0f, 9.0f, 7.0f});
}

TEST(Vec3ArithmeticTest, MultiplicationVector) {
    Vec3 a{10.0f, 6.0f, 12.0f};
    Vec3 b{0.5f, 2.0f, 1.5f};
    vec3_expect_near((a * b), Vec3{5.0f, 12.0f, 18.0f});
}

TEST(Vec3ArithmeticTest, DivisionVector) {
    Vec3 a{10.0f, 6.0f, 12.0f};
    Vec3 b{2.0f, 3.0f, 4.0f};
    vec3_expect_near((a / b), Vec3{5.0f, 2.0f, 3.0f});
}

TEST(Vec3ArithmeticTest, MultiplicationScalar) {
    Vec3 a{10.0f, 6.0f, 12.0f};
    vec3_expect_near((a * 2), Vec3{20.0f, 12.0f, 24.0f});
}

TEST(Vec3ArithmeticTest, DivisionScalar) {
    Vec3 a{9.0f, 6.0f, 12.0f};
    vec3_expect_near((a / 3), Vec3{3.0f, 2.0f, 4.0f});
}

/* comparisons */

TEST(Vec3ComparisonTest, IsEqual) {
    Vec3 a{120.0f, 20.0f, 12.0f};
    Vec3 b{120.0f, 10.0f, 12.0f};
    EXPECT_EQ(a == a, true);
    EXPECT_EQ(a == b, false);
}

TEST(Vec3ComparisonTest, IsNotEqual) {
    Vec3 a{120.0f, 20.0f, 12.0f};
    Vec3 b{120.0f, 10.0f, 12.0f};
    EXPECT_EQ(a != a, false);
    EXPECT_EQ(a != b, true);
}

/* Length + Normalization */

TEST(Vec3LengthTest, Length) {
    Vec3 a{16.0f, 2.0f, 3.0f};

    float length = a.length();
    EXPECT_NEAR(length, 16.4012194f, EPSILON);

    float length_squared = a.length_squared();
    EXPECT_FLOAT_EQ(length_squared, 269.0f);
    EXPECT_NEAR((length * length), length_squared, EPSILON);
}

TEST(Vec3LengthTest, LengthZero) {
    Vec3 a{};
    EXPECT_FLOAT_EQ(a.length(), 0.0f);
}

TEST(Vec3LengthTest, Normalization) {
    constexpr Vec3 base{16.0f, 2.0f, 3.0f};

    vec3_expect_near(base.normalized(), {0.975537f, 0.121942f, 0.182913f});
    vec3_expect_near(base.normalized() * base.length(), base);

    Vec3 v = base;
    v.normalize();
    vec3_expect_near(v, {0.975537f, 0.121942f, 0.182913f});
}

TEST(Vec3LengthTest, NormalizedZero) {
    Vec3 v{};
    vec3_expect_near(v.normalized(), Vec3{});
}

TEST(Vec3LengthTest, NormalizeVec3Zero) {
    Vec3 v{};
    v.normalize();
    vec3_expect_near(v, Vec3{});
}

TEST(Vec3LengthTest, Distance) {
    Vec3 a{4.0f, 2.0f, 2.0f};
    Vec3 b{6.0f, 3.0f, 4.0f};

    float dist = a.distance(b);
    EXPECT_NEAR(dist, 3.0f, EPSILON);
    EXPECT_NEAR(a.distance_squared(b), dist * dist, EPSILON);
}

TEST(Vec3LengthTest, SameDistance) {
    Vec3 a{4.0f, 2.0f, 3.0f};
    EXPECT_FLOAT_EQ(a.distance(a), 0.0f);
    EXPECT_FLOAT_EQ(a.distance_squared(a), 0.0f);
}

TEST(Vec3LengthTest, LimitLengthHasExceeded) {
    Vec3 v{4.0f, 1.0f, 3.0f};
    float length = 3.0f;

    Vec3 len = v.limited_length(length);
    Vec3 expected = v.normalized() * length;
    vec3_expect_near(len, expected);
}

TEST(Vec3LengthTest, LimitLengthHasNotExceeded) {
    Vec3 v{4.0f, 1.0f, 3.0f};
    Vec3 len = v.limited_length(50.0f);
    vec3_expect_near(len, v);
}

TEST(Vec3LengthTest, LimitLengthToZeroAndUnder) {
    Vec3 v{4.0f, 1.0f, 3.0f};
    Vec3 len = v.limited_length(0.0f);
    vec3_expect_near(len, {});

    len = v.limited_length(-4.0f);
    vec3_expect_near(len, {});
}

/* dot & cross product */

TEST(Vec3ProductTest, DotProduct) {
    Vec3 a{1.0f, 2.0f, 3.0f};
    Vec3 b{3.0f, 4.0f, 2.0f};
    EXPECT_NEAR(a.dot(b), 17.0f, EPSILON);
}

TEST(Vec3ProductTest, CrossProduct) {
    Vec3 a{1.0f, 2.0f, 0.0f};
    Vec3 b{3.0f, 4.0f, 0.0f};
    vec3_expect_near(a.cross(b), {0.0f, 0.0f, -2.0f});
}

TEST(Vec3ProductTest, Absolute) {
    Vec3 v{-3.0f, 2.0f, -2.0f};
    vec3_expect_near(v.abs(), {3.0f, 2.0f, 2.0f});
}

/* unary operators */

TEST(Vec3UnaryTest, Minus) {
    Vec3 v{-3.0f, 2.0f, 8.0f};
    vec3_expect_near(-v, {3.0f, -2.0f, -8.0f});
}

TEST(Vec3UnaryTest, Plus) {
    Vec3 v{-3.0f, 2.0f, 8.0f};
    vec3_expect_near(+v, {-3.0f, 2.0f, 8.0f});
}

/* identity tests */

TEST(Vec3IdentityTest, NormalizedIsUnitLength) {
    Vec3 v{-3.0f, 2.0f, 8.0f};
    EXPECT_FLOAT_EQ(v.normalized().length(), 1.0f);

    v.normalize();
    EXPECT_FLOAT_EQ(v.length(), 1.0f);
}
