#pragma once

#include "core/math/color.h"
#include "core/math/vec2.h"
#include "core/math/vec2i.h"

#include "render_handles.h"

class RenderQueue;

/**
 * @class RenderQueueBuilder
 * @brief Populates a RenderQueue using high-level commands.
 */
class RenderQueueBuilder {
public:
    explicit RenderQueueBuilder(RenderQueue &queue)
        : m_queue(queue) {}

    /* ================ *
     *     Textures     *
     * ================ */
    void draw_texture_ex(TextureHandle texture, Color color, const Vec2i &position, const Vec2i &size, const Vec2 &anchor, float rotation);
    void draw_texture_blended(TextureHandle texture, Color color, const Vec2i &position, const Vec2i &size);
    void draw_texture(TextureHandle texture, const Vec2i &position, const Vec2i &size);

    /* ================ *
     *   Solid Colors   *
     * ================ */
    void draw_color_ex(Color color, const Vec2i &position, const Vec2i &size, const Vec2 &anchor, float rotation);
    void draw_color(Color color, const Vec2i &position, const Vec2i &size);

public:
    /* FIXME: this function is only needed so that the renderer
     * can get its queue from a RenderContext object. */
    [[nodiscard]] const RenderQueue &queue() const { return m_queue; }

private:
    RenderQueue &m_queue;
};
