#include "core/math/vec2i.h"

#include <gtest/gtest.h>

constexpr float EPSILON = 0.001f;

static inline void vec2i_eq(const Vec2i &a, const Vec2i &b) {
    EXPECT_EQ(a.x, b.x);
    EXPECT_EQ(a.y, b.y);
}

/* Constructor tests */

TEST(Vec2iConstructorTest, DefaultConstructor) {
    Vec2i v;
    EXPECT_EQ(v.x, 0);
    EXPECT_EQ(v.y, 0);
}

TEST(Vec2iConstructorTest, ValueConstructor) {
    Vec2i v{3, 4};
    EXPECT_EQ(v.x, 3);
    EXPECT_EQ(v.y, 4);
}

/* Arithmetic Operations */

TEST(Vec2iArithmeticTest, Addition) {
    Vec2i a{10, 6};
    Vec2i b{2, -3};
    vec2i_eq((a + b), Vec2i{12, 3});
}

TEST(Vec2iArithmeticTest, Subtraction) {
    Vec2i a{10, 6};
    Vec2i b{2, -3};
    vec2i_eq((a - b), Vec2i{8, 9});
}

TEST(Vec2iArithmeticTest, MultiplicationVector) {
    Vec2i a{10, 6};
    Vec2i b{1, 2};
    vec2i_eq((a * b), Vec2i{10, 12});
}

TEST(Vec2iArithmeticTest, DivisionVector) {
    Vec2i a{10, 6};
    Vec2i b{2, 3};
    vec2i_eq((a / b), Vec2i{5, 2});
}

TEST(Vec2iArithmeticTest, MultiplicationScalar) {
    Vec2i a{10, 6};
    vec2i_eq((a * 2), Vec2i{20, 12});
}

TEST(Vec2iArithmeticTest, DivisionScalar) {
    Vec2i a{9, 6};
    vec2i_eq((a / 3), Vec2i{3, 2});
}

/* comparisons */

TEST(Vec2iComparisonTest, IsEqual) {
    Vec2i a{120, 20};
    Vec2i b{120, 10};
    EXPECT_EQ(a == a, true);
    EXPECT_EQ(a == b, false);
}

TEST(Vec2iComparisonTest, IsNotEqual) {
    Vec2i a{120, 20};
    Vec2i b{120, 10};
    EXPECT_EQ(a != a, false);
    EXPECT_EQ(a != b, true);
}

/* Length + Normalization */

TEST(Vec2iLengthTest, Length) {
    Vec2i a{16, 2};

    float length = a.length();
    EXPECT_NEAR(length, 16.12451f, EPSILON);

    int length_squared = a.length_squared();
    EXPECT_EQ(length_squared, 260);
    EXPECT_NEAR((length * length), length_squared, EPSILON);
}

TEST(Vec2iLengthTest, LengthZero) {
    Vec2i a{};
    EXPECT_FLOAT_EQ(a.length(), 0);
}

/* absolute product */

TEST(Vec2iProductTest, Absolute) {
    Vec2i v{-3, 2};
    vec2i_eq(v.abs(), {3, 2});
}

/* unary operators */

TEST(Vec2iUnaryTest, Minus) {
    Vec2i v{-3, 2};
    vec2i_eq(-v, {3, -2});
}

TEST(Vec2iUnaryTest, Plus) {
    Vec2i v{-3, 2};
    vec2i_eq(+v, {-3, 2});
}
