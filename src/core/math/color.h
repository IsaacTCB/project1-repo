#pragma once

#include <cstdint>
#include <format>
#include <string>
#include <tuple>

/* TODO: this class is still missing uint32 hex rgba conversion.
 * also hue functionality would be nice. */
class Color {
public:
    uint8_t r{0xFF};
    uint8_t g{0xFF};
    uint8_t b{0xFF};
    uint8_t a{0xFF};

public:
    constexpr explicit Color() = default;
    constexpr explicit Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFF)
        : r(r), g(g), b(b), a(a) {}

    constexpr explicit Color(uint32_t rgba32)
        : r((rgba32 >> 24) & 0xFF),
          g((rgba32 >> 16) & 0xFF),
          b((rgba32 >> 8) & 0xFF),
          a(rgba32 & 0xFF) {}

    [[nodiscard]]
    std::tuple<float, float, float, float> normalized() const {
        /* TODO: replace tuple with vec4 once that gets implemented */
        return {
            static_cast<float>(r) / static_cast<float>(0xFF),
            static_cast<float>(g) / static_cast<float>(0xFF),
            static_cast<float>(b) / static_cast<float>(0xFF),
            static_cast<float>(a) / static_cast<float>(0xFF),
        };
    }

    [[nodiscard]]
    std::string get_hex_string(bool uppercase = true) const {
        return uppercase ? std::format("0x{:02X}{:02X}{:02X}{:02X}", r, g, b, a)
                         : std::format("0x{:02x}{:02x}{:02x}{:02x}", r, g, b, a);
    }

public:
    static const Color WHITE;
    static const Color BLACK;
    static const Color RED;
    static const Color GREEN;
    static const Color BLUE;
};

inline const Color Color::WHITE{0xFF, 0xFF, 0xFF};
inline const Color Color::BLACK{0x00, 0x00, 0x00};
inline const Color Color::RED{0xFF, 0x00, 0x00};
inline const Color Color::GREEN{0x00, 0xFF, 0x00};
inline const Color Color::BLUE{0x00, 0x00, 0xFF};

static_assert(sizeof(Color) == sizeof(uint8_t) * 4);

template <>
struct std::formatter<Color> : std::formatter<std::string_view> {
    auto format(const Color &color, format_context &ctx) const {
        std::string s = color.get_hex_string();
        return formatter<std::string_view>::format(s, ctx);
    }
};
