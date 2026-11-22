#pragma once

#include <span>
#include <string_view>
#include <vector>

#include "font_resource.h"
#include "runtime/renderer/render_glyphs.h"

class TextLayout {
public:
    static constexpr char MISSING_CHAR = '?';

    [[nodiscard]] std::span<const GlyphQuad> glyphs() const {
        return {m_quads.data(), m_quads.size()};
    }

    void build(const FontResource &font, std::string_view string) {
        extract_glyphs_from_string(font, string);
        build_glyph_quads();
    }

private:
    void extract_glyphs_from_string(const FontResource &font, std::string_view string) {
        m_glyphs.clear();
        m_glyphs.reserve(string.size());

        const auto *missing_glyph = font.glyph(MISSING_CHAR);

        char previous = '\0';
        float cursor_x = 0.0f;
        for (char ch : string) {
            /* TODO: implement unicode decoding support */
            const auto *glyph = font.glyph(ch);
            if (!glyph && missing_glyph) {
                ch = MISSING_CHAR;
                glyph = missing_glyph;
            }

            if (!glyph) {
                continue;
            }

            /* apply kerning offset */
            float advance = glyph->advance;
            if (previous != '\0') {
                advance += font.get_kerning(ch, previous);
            }

            m_glyphs.emplace_back(
                glyph,
                cursor_x);

            previous = ch;
            cursor_x += advance;
        }
    }

    void build_glyph_quads() {
        m_quads.clear();
        m_quads.reserve(m_glyphs.size());

        for (const auto &glyph_def : m_glyphs) {
            const auto *glyph = glyph_def.glyph;
            if (glyph->size.x >= 1.0f && glyph->size.y >= 1.0f) {
                m_quads.emplace_back(GlyphQuad{
                    .uv_min = glyph->uv_min,
                    .uv_max = glyph->uv_max,
                    .position = Vec2{glyph_def.x_pos, 0.0f} + glyph->bearing,
                    .size = glyph->size,
                });
            }
        }
    }

private:
    std::vector<GlyphQuad> m_quads;

    struct GlyphDef {
        const FontResource::Glyph *glyph;
        float x_pos;
    };
    std::vector<GlyphDef> m_glyphs;
};
