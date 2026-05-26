#include "core/math/vec3i.h"

#include "core/math/vec2i.h"
#include "core/math/vec3.h"

#include <gtest/gtest.h>

constexpr float EPSILON = 0.001f;

static inline void vec3i_eq(const Vec3i &a, const Vec3i &b) {
    EXPECT_EQ(a.x, b.x);
    EXPECT_EQ(a.y, b.y);
    EXPECT_EQ(a.z, b.z);
}

/* Size tests */

TEST(Vec3iSizeTest, EnsureSize) {
    EXPECT_EQ(sizeof(Vec3i), sizeof(int) * 3);
}

/* Constructor tests */

TEST(Vec3iConstructorTest, DefaultConstructor) {
    Vec3i v;
    EXPECT_EQ(v.x, 0);
    EXPECT_EQ(v.y, 0);
    EXPECT_EQ(v.z, 0);
}

TEST(Vec3iConstructorTest, ValueConstructor) {
    Vec3i v{3, 4, 2};
    EXPECT_EQ(v.x, 3);
    EXPECT_EQ(v.y, 4);
    EXPECT_EQ(v.z, 2);
}

TEST(Vec3iConstructorTest, FloatConversionConstructor) {
    Vec3 a{4.0f, 2.0f, 4.2f};
    Vec3i b{a};
    EXPECT_EQ(b.x, (int)a.x);
    EXPECT_EQ(b.y, (int)a.y);
    EXPECT_EQ(b.z, (int)a.z);
}

TEST(Vec3iConstructorTest, Vec2iConversionConstructor) {
    Vec2i v1{4, 3};
    Vec3i v2{v1, 6};
    vec3i_eq(v2, {v1.x, v1.y, 6});
}

/* Arithmetic Operations */

TEST(Vec3iArithmeticTest, Addition) {
    Vec3i a{10, 6, 4};
    Vec3i b{2, -3, 2};
    vec3i_eq((a + b), Vec3i{12, 3, 6});
}

TEST(Vec3iArithmeticTest, Subtraction) {
    Vec3i a{10, 6, 4};
    Vec3i b{2, -3, 2};
    vec3i_eq((a - b), Vec3i{8, 9, 2});
}

TEST(Vec3iArithmeticTest, MultiplicationVector) {
    Vec3i a{10, 6, 3};
    Vec3i b{1, 2, 3};
    vec3i_eq((a * b), Vec3i{10, 12, 9});
}

TEST(Vec3iArithmeticTest, DivisionVector) {
    Vec3i a{10, 6, 3};
    Vec3i b{2, 3, 3};
    vec3i_eq((a / b), Vec3i{5, 2, 1});
}

TEST(Vec3iArithmeticTest, MultiplicationScalar) {
    Vec3i a{10, 6, 3};
    vec3i_eq((a * 2), Vec3i{20, 12, 6});
}

TEST(Vec3iArithmeticTest, DivisionScalar) {
    Vec3i a{9, 6, 3};
    vec3i_eq((a / 3), Vec3i{3, 2, 1});
}

/* comparisons */

TEST(Vec3iComparisonTest, IsEqual) {
    Vec3i a{120, 20, 12};
    Vec3i b{120, 10, 12};
    EXPECT_EQ(a == a, true);
    EXPECT_EQ(a == b, false);
}

TEST(Vec3iComparisonTest, IsNotEqual) {
    Vec3i a{120, 20, 12};
    Vec3i b{120, 10, 12};
    EXPECT_EQ(a != a, false);
    EXPECT_EQ(a != b, true);
}

/* Length + Normalization */

TEST(Vec3iLengthTest, Length) {
    Vec3i a{16, 2, 3};

    float length = a.length();
    EXPECT_NEAR(length, 16.4012194f, EPSILON);

    int length_squared = a.length_squared();
    EXPECT_EQ(length_squared, 269);
    EXPECT_NEAR((length * length), length_squared, EPSILON);
}

TEST(Vec3iLengthTest, LengthZero) {
    Vec3i a{};
    EXPECT_FLOAT_EQ(a.length(), 0);
}

/* absolute product */

TEST(Vec3iProductTest, Absolute) {
    Vec3i v{-3, 2, -2};
    vec3i_eq(v.abs(), {3, 2, 2});
}

/* unary operators */

TEST(Vec3iUnaryTest, Minus) {
    Vec3i v{-3, 2, -2};
    vec3i_eq(-v, {3, -2, 2});
}

TEST(Vec3iUnaryTest, Plus) {
    Vec3i v{-3, 2, -2};
    vec3i_eq(+v, {-3, 2, -2});
}
