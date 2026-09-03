#pragma once

#include <variant>
#include <vector>

#include "core/math/color.h"
#include "core/math/vec2.h"
#include "core/math/vec2i.h"

#include "render_handles.h"

struct RenderCommandDraw {
    TextureHandle texture; /* handle is null = draw solid color */
    Vec2 pos;
    Vec2 size;
    Vec2 anchor;
    float rotation;
    Color color;
    // int zsort;
};
using RenderCommand = std::variant<RenderCommandDraw>;

class RenderQueue {
public:
    RenderQueue(size_t initial_size = 1000) {
        m_queue.reserve(initial_size);
    }

    [[nodiscard]]
    const std::vector<RenderCommand> &get() const {
        return m_queue;
    }

    void clear() {
        m_queue.clear();
    }

public:
    void draw_texture_ex(TextureHandle texture, Color color, const Vec2i &position, const Vec2i &size, const Vec2 &anchor, float rotation) {
        m_queue.emplace_back(RenderCommandDraw{
            .texture = texture,
            .pos = Vec2{position},
            .size = Vec2{size},
            .anchor = anchor,
            .rotation = rotation,
            .color = color,
            // .zsort = 0,
        });
    }

    void draw_texture_blended(TextureHandle texture, Color color, const Vec2i &position, const Vec2i &size) {
        draw_texture_ex(texture, color, position, size, {0.0f, 0.0f}, 0.0f);
    }

    void draw_texture(TextureHandle texture, const Vec2i &position, const Vec2i &size) {
        draw_texture_blended(texture, Color::WHITE, position, size);
    }

    void draw_color(Color color, const Vec2i &position, const Vec2i &size) {
        draw_texture_blended(Handle{}, color, position, size);
    }

private:
    std::vector<RenderCommand> m_queue;
};
