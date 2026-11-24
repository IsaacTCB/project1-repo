#pragma once

#include <cstdint>
#include <unordered_map>

#include "core/math/vec2.h"
#include "runtime/renderer/render_handles.h"

class FontResource {
public:
    /* standard ASCII font range */
    static constexpr int FIRST_CHAR = 32;
    static constexpr int CHAR_COUNT = 95;

public:
    using Codepoint = char32_t;

    struct Metrics {
        float ascent;
        float descent;
        float line_gap;
    } metrics;

    struct Glyph {
        Vec2 uv_min, uv_max;
        Vec2 size, bearing;
        float advance;
    };

    /* FIXME: atlas texture is leaking, because no one unloads it */
    TextureHandle atlas;

    [[nodiscard]] bool valid() const noexcept {
        return this->atlas.valid();
    }
    [[nodiscard]] explicit operator bool() const noexcept {
        return valid();
    }

public:
    Glyph &add_glyph(Codepoint code, const Glyph &glyph) {
        return m_glyphs.emplace(code, glyph).first->second;
    }

    [[nodiscard]] const Glyph *glyph(Codepoint code) const {
        const auto it = m_glyphs.find(code);
        if (it == m_glyphs.end()) {
            return nullptr;
        }
        return &it->second;
    }

    [[nodiscard]] Glyph *glyph(Codepoint code) {
        const auto *const_this = const_cast<const FontResource *>(this);
        return const_cast<Glyph *>(const_this->glyph(code));
    }

    static uint64_t make_kerning_key(Codepoint current, Codepoint previous) {
        return static_cast<uint64_t>(previous) << 32 | static_cast<uint64_t>(current);
    }

    void set_kerning(Codepoint current, Codepoint previous, float kern) {
        uint64_t key = make_kerning_key(current, previous);
        m_kerning.emplace(key, kern);
    }

    float get_kerning(Codepoint current, Codepoint previous) const {
        const auto it = m_kerning.find(make_kerning_key(current, previous));
        if (it == m_kerning.end()) {
            return 0.0f;
        }
        return it->second;
    }

private:
    std::unordered_map<Codepoint, Glyph> m_glyphs;
    std::unordered_map<uint64_t, float> m_kerning;
};
