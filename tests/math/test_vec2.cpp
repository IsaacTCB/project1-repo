#include "core/math/vec2.h"

#include <gtest/gtest.h>

constexpr float EPSILON = 0.001f;

static inline void vec2_expect_near(const Vec2 &a, const Vec2 &b, float epsilon = EPSILON) {
    EXPECT_NEAR(a.x, b.x, epsilon);
    EXPECT_NEAR(a.y, b.y, epsilon);
}

/* Constructor tests */

TEST(Vec2ConstructorTest, DefaultConstructor) {
    Vec2 v;
    EXPECT_FLOAT_EQ(v.x, 0.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
}

TEST(Vec2ConstructorTest, ValueConstructor) {
    Vec2 v{3.0f, 4.0f};
    EXPECT_FLOAT_EQ(v.x, 3.0f);
    EXPECT_FLOAT_EQ(v.y, 4.0f);
}

/* Arithmetic Operations */

TEST(Vec2ArithmeticTest, Addition) {
    Vec2 a{10.0f, 6.0f};
    Vec2 b{2.0f, -3.0f};
    vec2_expect_near((a + b), Vec2{12.0f, 3.0f});
}

TEST(Vec2ArithmeticTest, Subtraction) {
    Vec2 a{10.0f, 6.0f};
    Vec2 b{2.0f, -3.0f};
    vec2_expect_near((a - b), Vec2{8.0f, 9.0f});
}

TEST(Vec2ArithmeticTest, MultiplicationVector) {
    Vec2 a{10.0f, 6.0f};
    Vec2 b{0.5f, 2.0f};
    vec2_expect_near((a * b), Vec2{5.0f, 12.0f});
}

TEST(Vec2ArithmeticTest, DivisionVector) {
    Vec2 a{10.0f, 6.0f};
    Vec2 b{2.0f, 3.0f};
    vec2_expect_near((a / b), Vec2{5.0f, 2.0f});
}

TEST(Vec2ArithmeticTest, MultiplicationScalar) {
    Vec2 a{10.0f, 6.0f};
    vec2_expect_near((a * 2), Vec2{20.0f, 12.0f});
}

TEST(Vec2ArithmeticTest, DivisionScalar) {
    Vec2 a{9.0f, 6.0f};
    vec2_expect_near((a / 3), Vec2{3.0f, 2.0f});
}

/* comparisons */

TEST(Vec2ComparisonTest, IsEqual) {
    Vec2 a{120.0f, 20.0f};
    Vec2 b{120.0f, 10.0f};
    EXPECT_EQ(a == a, true);
    EXPECT_EQ(a == b, false);
}

TEST(Vec2ComparisonTest, IsNotEqual) {
    Vec2 a{120.0f, 20.0f};
    Vec2 b{120.0f, 10.0f};
    EXPECT_EQ(a != a, false);
    EXPECT_EQ(a != b, true);
}

/* Length + Normalization */

TEST(Vec2LengthTest, Length) {
    Vec2 a{16.0f, 2.0f};

    float length = a.length();
    EXPECT_NEAR(length, 16.12451f, EPSILON);

    float length_squared = a.length_squared();
    EXPECT_FLOAT_EQ(length_squared, 260.0f);
    EXPECT_NEAR((length * length), length_squared, EPSILON);
}

TEST(Vec2LengthTest, LengthZero) {
    Vec2 a{};
    EXPECT_FLOAT_EQ(a.length(), 0.0f);
}

TEST(Vec2LengthTest, Normalization) {
    constexpr Vec2 base{16.0f, 2.0f};

    vec2_expect_near(base.normalized(), {0.99227f, 0.12403f});
    vec2_expect_near(base.normalized() * base.length(), base);

    Vec2 v = base;
    v.normalize();
    vec2_expect_near(v, {0.99227f, 0.12403f});
}

TEST(Vec2LengthTest, NormalizedZero) {
    Vec2 v{};
    vec2_expect_near(v.normalized(), Vec2{});
}

TEST(Vec2LengthTest, NormalizeVec2Zero) {
    Vec2 v{};
    v.normalize();
    vec2_expect_near(v, Vec2{});
}

TEST(Vec2LengthTest, Distance) {
    Vec2 a{4.0f, 2.0f};
    Vec2 b{6.0f, 3.0f};

    float dist = a.distance(b);
    EXPECT_NEAR(dist, 2.236067f, EPSILON);
    EXPECT_NEAR(a.distance_squared(b), dist * dist, EPSILON);
}

TEST(Vec2LengthTest, SameDistance) {
    Vec2 a{4.0f, 2.0f};
    EXPECT_FLOAT_EQ(a.distance(a), 0.0f);
    EXPECT_FLOAT_EQ(a.distance_squared(a), 0.0f);
}

TEST(Vec2LengthTest, LimitLengthHasExceeded) {
    Vec2 v{4.0f, 5.0f};
    float length = 3.0f;

    Vec2 len = v.limited_length(length);
    Vec2 expected = v.normalized() * length;
    vec2_expect_near(len, expected);
}

TEST(Vec2LengthTest, LimitLengthHasNotExceeded) {
    Vec2 v{4.0f, 5.0f};
    Vec2 len = v.limited_length(50.0f);
    vec2_expect_near(len, v);
}

TEST(Vec2LengthTest, LimitLengthToZeroAndUnder) {
    Vec2 v{4.0f, 5.0f};
    Vec2 len = v.limited_length(0.0f);
    vec2_expect_near(len, {});

    len = v.limited_length(-4.0f);
    vec2_expect_near(len, {});
}

/* dot & cross product */

TEST(Vec2ProductTest, DotProduct) {
    Vec2 a{1.0f, 2.0f};
    Vec2 b{3.0f, 4.0f};
    EXPECT_NEAR(a.dot(b), 11.0f, EPSILON);
}

TEST(Vec2ProductTest, CrossProduct) {
    Vec2 a{1.0f, 2.0f};
    Vec2 b{3.0f, 4.0f};
    EXPECT_NEAR(a.cross(b), -2.0f, EPSILON);
}

TEST(Vec2ProductTest, Absolute) {
    Vec2 v{-3.0f, 2.0f};
    vec2_expect_near(v.abs(), {3.0f, 2.0f});
}

/* unary operators */

TEST(Vec2UnaryTest, Minus) {
    Vec2 v{-3.0f, 2.0f};
    vec2_expect_near(-v, {3.0f, -2.0f});
}

TEST(Vec2UnaryTest, Plus) {
    Vec2 v{-3.0f, 2.0f};
    vec2_expect_near(+v, {-3.0f, 2.0f});
}

/* identity tests */

TEST(Vec2IdentityTest, NormalizedIsUnitLength) {
    Vec2 v{-3.0f, 2.0f};
    EXPECT_FLOAT_EQ(v.normalized().length(), 1.0f);

    v.normalize();
    EXPECT_FLOAT_EQ(v.length(), 1.0f);
}
