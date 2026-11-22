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
    void extract_glyphs_from_string(const FontResource &font, std::string_view string, TextAlign align);
    void build_glyph_quads();

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
