#pragma once

#include <variant>
#include <vector>

#include "core/math/color.h"
#include "core/math/vec2i.h"
#include "render_handles.h"

struct RenderCommandDraw {
    TextureHandle texture; /* handle is null = draw solid color */
    float x, y, width, height;
    Color color;
    // int zsort;
};
using RenderCommand = std::variant<RenderCommandDraw>;

class RenderQueue {
public:
    RenderQueue() {
        m_queue.reserve(1000);
    }

    void clear() {
        m_queue.clear();
    }

    [[nodiscard]]
    const std::vector<RenderCommand> &get() const {
        return m_queue;
    }

    void draw_texture(TextureHandle texture, const Vec2i &position, const Vec2i &size) {
        m_queue.emplace_back(RenderCommandDraw{
            .texture = texture,
            .x = static_cast<float>(position.x),
            .y = static_cast<float>(position.y),
            .width = static_cast<float>(size.x),
            .height = static_cast<float>(size.y),
            .color = Color::WHITE,
            // .zsort = 0,
        });
    }

    void draw_color(Color color, const Vec2i &position, const Vec2i &size) {
        m_queue.emplace_back(RenderCommandDraw{
            .texture = Handle{},
            .x = static_cast<float>(position.x),
            .y = static_cast<float>(position.y),
            .width = static_cast<float>(size.x),
            .height = static_cast<float>(size.y),
            .color = color,
            // .zsort = 0,
        });
    }

private:
    std::vector<RenderCommand> m_queue;
};

#define RGBA8(r, g, b, a) (r & 0xFF) << 24 | (g & 0xFF) << 16 | (b & 0xFF) << 8 | (a & 0xFF)
#define RGB8(r, g, b) RGBA8(r, g, b, 0xFF)
