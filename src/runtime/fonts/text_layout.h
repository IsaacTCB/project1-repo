#pragma once

#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

#include "core/math/vec2.h"
#include "font_resource.h"
#include "runtime/renderer/render_glyphs.h"
#include "runtime/renderer/render_handles.h"

enum class TextAlign : uint8_t {
    LEFT = 0,
    CENTER,
    RIGHT,
};

struct TextLayoutParams {
    TextAlign align = TextAlign::LEFT;
};

class TextLayout {
public:
    static constexpr char MISSING_CHAR = '?';

    [[nodiscard]] std::span<const GlyphQuad> glyphs() const {
        return {m_quads.data(), m_quads.size()};
    }

    [[nodiscard]] const Vec2 &size() const { return m_size; }
    [[nodiscard]] TextureHandle atlas() const { return m_atlas; }

    void build(const FontResource &font, std::string_view string, const TextLayoutParams params = {}) {
        extract_glyphs_from_string(font, string, params.align);
        build_glyph_quads();
    }

private:
    void extract_glyphs_from_string(const FontResource &font, std::string_view string, TextAlign align) {
        m_lines.clear();
        m_glyphs.clear();
        m_glyphs.reserve(string.size());

        /* bind texture */
        m_atlas = font.atlas;

        float ascent = font.metrics.ascent;
        float line_height =
            (font.metrics.ascent - font.metrics.descent + font.metrics.line_gap);

        const auto *missing_glyph = font.glyph(MISSING_CHAR);

        char previous = '\0';
        float cursor_x = 0.0f;

        size_t line_begin = 0;
        float line_width = 0;
        for (size_t i = 0; i <= string.size(); ++i) {
            if (i == string.size() || string[i] == '\n') {
                const float baseline_y = ascent + (static_cast<float>(m_lines.size()) * line_height);
                m_lines.push_back(Line{
                    .begin = line_begin,
                    .end = m_glyphs.size(),
                    .width = line_width,
                    .x_pos = 0.0f, /* x position offset is updated later */
                    .y_pos = baseline_y,
                });
                line_begin = m_glyphs.size();
                line_width = 0.0f;
                cursor_x = 0.0f;
                continue;
            }

            if (string[i] == '\r') {
                continue;
            }

            /* TODO: implement unicode decoding support */
            char ch = string[i];
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
            line_width += advance;
        }

        /* calculate size */
        m_size.x = 0.0f;
        for (const auto &line : m_lines) {
            m_size.x = std::max(m_size.x, line.width);
        }
        m_size.y = font.metrics.ascent - font.metrics.descent +
                   (line_height * (static_cast<float>(m_lines.size() - 1)));

        const auto get_offset_x = [](float box_width, float src_width, TextAlign align) -> float {
            switch (align) {
                case TextAlign::LEFT:
                default:
                    return 0.0f;
                case TextAlign::CENTER:
                    return (box_width - src_width) * 0.5f;
                case TextAlign::RIGHT:
                    return box_width - src_width;
            }
        };

        for (auto &line : m_lines) {
            line.x_pos = get_offset_x(m_size.x, line.width, align);
        }
    }

    void build_glyph_quads() {
        m_quads.clear();
        m_quads.reserve(m_glyphs.size());

        for (const auto &line : m_lines) {
            for (size_t i = line.begin; i < line.end; ++i) {
                const auto *glyph = m_glyphs[i].glyph;
                if (glyph->size.x >= 1.0f && glyph->size.y >= 1.0f) {
                    Vec2 pos = Vec2{m_glyphs[i].x_pos, 0.0f};
                    pos += Vec2{line.x_pos, line.y_pos};

                    m_quads.emplace_back(GlyphQuad{
                        .uv_min = glyph->uv_min,
                        .uv_max = glyph->uv_max,
                        .position = pos + glyph->bearing,
                        .size = glyph->size,
                    });
                }
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

    struct Line {
        size_t begin, end;
        float width, x_pos, y_pos;
    };
    std::vector<Line> m_lines;

    Vec2 m_size;
    TextureHandle m_atlas;
};
