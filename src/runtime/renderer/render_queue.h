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

    [[nodiscard]] const std::vector<RenderCommand> &get() const { return m_queue; }
    [[nodiscard]] std::vector<RenderCommand> &get() { return m_queue; }

    void clear() {
        m_queue.clear();
    }

private:
    std::vector<RenderCommand> m_queue;
};
