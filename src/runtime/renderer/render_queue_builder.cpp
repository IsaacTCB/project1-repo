#include "render_queue_builder.h"

#include "render_handles.h"
#include "render_queue.h"

/* ================ *
 *     Textures     *
 * ================ */

void RenderQueueBuilder::draw_texture_ex(
    TextureHandle texture, Color color, const Vec2i &position, const Vec2i &size, const Vec2 &anchor, float rotation) {
    m_queue.get().emplace_back(RenderCommandDraw{
        .texture = texture,
        .pos = Vec2{position},
        .size = Vec2{size},
        .anchor = anchor,
        .uv_min = Vec2{0.0f, 0.0f},
        .uv_max = Vec2{1.0f, 1.0f},
        .rotation = rotation,
        .color = color,
        // .zsort = 0,
    });
}

void RenderQueueBuilder::draw_texture_blended(TextureHandle texture, Color color, const Vec2i &position, const Vec2i &size) {
    draw_texture_ex(texture, color, position, size, {0.0f, 0.0f}, 0.0f);
}

void RenderQueueBuilder::draw_texture(TextureHandle texture, const Vec2i &position, const Vec2i &size) {
    draw_texture_blended(texture, Color::WHITE, position, size);
}

/* ================ *
 *   Solid Colors   *
 * ================ */

void RenderQueueBuilder::draw_color_ex(Color color, const Vec2i &position, const Vec2i &size, const Vec2 &anchor, float rotation) {
    draw_texture_ex(TextureHandle{}, color, position, size, anchor, rotation);
}

void RenderQueueBuilder::draw_color(Color color, const Vec2i &position, const Vec2i &size) {
    draw_color_ex(color, position, size, Vec2{0.0f, 0.0f}, 0.0f);
}

/* ================ *
 *     Sprite2D     *
 * ================ */
void RenderQueueBuilder::draw_sprite(const Sprite2D &sprite, const Vec2i &position, const Vec2 &scale, float rotation) {
    Vec2 size = sprite.size * scale;
    m_queue.get().emplace_back(RenderCommandDraw{
        .texture = sprite.texture.handle(),
        .pos = Vec2{position},
        .size = Vec2{size},
        .anchor = sprite.anchor,
        .uv_min = sprite.uv_min,
        .uv_max = sprite.uv_max,
        .rotation = rotation,
        .color{},
    });
}
