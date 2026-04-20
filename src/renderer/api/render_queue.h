#pragma once

#include "render_handles.h"

#include <optional>
#include <variant>
#include <vector>

struct RenderCommandDraw {
    std::optional<TextureHandle> texture;
    float x, y, width, height;
    uint32_t color;
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
    const std::vector<RenderCommand>& get() const {
        return m_queue;
    }

    void draw_texture(TextureHandle texture, int x, int y, int width, int height) {
        RenderCommandDraw draw{
            .texture = texture,
            .x = static_cast<float>(x),
            .y = static_cast<float>(y),
            .width = static_cast<float>(width),
            .height = static_cast<float>(height),
            .color = 0xFFFFFFFF,
            // .zsort = 0,
        };
        m_queue.emplace_back(draw);
    }

    void draw_color(uint32_t color, int x, int y, int width, int height) {
        RenderCommandDraw draw{
            .texture = std::nullopt,
            .x = static_cast<float>(x),
            .y = static_cast<float>(y),
            .width = static_cast<float>(width),
            .height = static_cast<float>(height),
            .color = color,
            // .zsort = 0,
        };
        m_queue.emplace_back(draw);
    }

private:
    std::vector<RenderCommand> m_queue;
};
