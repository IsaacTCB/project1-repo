#pragma once

#include <cstddef>
#include <span>
#include <variant>
#include <vector>

#include "core/math/color.h"
#include "core/math/vec2.h"
#include "core/math/vec2i.h"

#include "render_glyphs.h"
#include "render_handles.h"

struct GlyphSpan {
    size_t offset;
    size_t size;
};

struct RenderCommandDraw {
    TextureHandle texture; /* handle is null = draw solid color */
    Vec2 pos;
    Vec2 size;
    Vec2 anchor;
    Vec2 uv_min;
    Vec2 uv_max;
    float rotation;
    Color color;
    // int zsort;
};
struct RenderCommandDrawText {
    TextureHandle texture;
    GlyphSpan glyphs;
    Vec2 pos;
    Vec2 size;
    Vec2 scale;
    Vec2 anchor;
    float rotation;
    Color color;
};
using RenderCommand = std::variant<
    RenderCommandDraw,
    RenderCommandDrawText>;

class RenderQueue {
public:
    RenderQueue(size_t initial_size = 1000) {
        m_queue.reserve(initial_size);
        m_glyphs.reserve(initial_size);
    }

    [[nodiscard]] const std::vector<RenderCommand> &get() const { return m_queue; }
    [[nodiscard]] std::vector<RenderCommand> &get() { return m_queue; }

    [[nodiscard]] GlyphSpan push_glyphs(std::span<const GlyphQuad> glyphs) {
        size_t offset = m_glyphs.size();
        for (const auto &glyph : glyphs) {
            m_glyphs.push_back(glyph);
        }
        return {
            .offset = offset,
            .size = m_glyphs.size() - offset,
        };
    }

    [[nodiscard]] std::span<const GlyphQuad> to_span(GlyphSpan glyphs) const {
        return std::span{
            m_glyphs.data() + glyphs.offset,
            m_glyphs.size(),
        };
    }

    void clear() {
        m_queue.clear();
        m_glyphs.clear();
    }

private:
    std::vector<RenderCommand> m_queue;
    std::vector<GlyphQuad> m_glyphs;
};
