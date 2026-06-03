#include "core/math/color.h"

#include <cstdint>
#include <gtest/gtest.h>

static inline void color_eq(const Color &a, const Color &b) {
    EXPECT_EQ(a.r, b.r);
    EXPECT_EQ(a.g, b.g);
    EXPECT_EQ(a.b, b.b);
    EXPECT_EQ(a.a, b.a);
}

/* Size tests */

TEST(ColorSizeTest, EnsureSize) {
    EXPECT_EQ(sizeof(Color), sizeof(uint8_t) * 4);
}

/* Constructor tests */

TEST(ColorConstructorTest, DefaultConstructorIsWhiteAndOpaque) {
    Color color{};

    /* ensure white */
    color_eq(color, Color{0xFF, 0xFF, 0xFF, 0xFF});

    /* also check if it matches with named constant */
    color_eq(color, Color::WHITE);
}

TEST(ColorConstructorTest, ValueConstructor) {
    Color color{0xFF, 0x00, 0x20, 0x20};
    color_eq(color, Color{0xFF, 0x00, 0x20, 0x20});
}

TEST(ColorConstructorTest, ValueConstructorNoAlpha) {
    Color color{0xFF, 0x00, 0x20};
    color_eq(color, Color{0xFF, 0x00, 0x20, 0xFF});
}

/* Operqation tests */

TEST(ColorOperationTest, NormalizedReturnsExpectedValues) {
    Color color{
        (uint8_t)(0.5f * (float)0xFF),
        (uint8_t)(0.2f * (float)0xFF),
        (uint8_t)(0.8f * (float)0xFF),
        (uint8_t)(1.0f * (float)0xFF),
    };

    auto [red_b, green_b, blue_b, alpha_b] = color.normalized();
    EXPECT_FLOAT_EQ(red_b, (float)color.r / (float)0xFF);
    EXPECT_FLOAT_EQ(green_b, (float)color.g / (float)0xFF);
    EXPECT_FLOAT_EQ(blue_b, (float)color.b / (float)0xFF);
    EXPECT_FLOAT_EQ(alpha_b, (float)color.a / (float)0xFF);
}

TEST(ColorOperationTest, HexStringReturnsExpectedString) {
    Color color{0xFF, 0x00, 0x20, 0x20};

    std::string hex = color.get_hex_string(true);
    EXPECT_STREQ(hex.c_str(), "0xFF002020");
    hex = color.get_hex_string(false);
    EXPECT_STREQ(hex.c_str(), "0xff002020");
}
