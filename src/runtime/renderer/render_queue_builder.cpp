#include "render_queue_builder.h"

#include "render_handles.h"
#include "render_queue.h"

/* ================ *
 *     Textures     *
 * ================ */

void RenderQueueBuilder::draw_texture_ex(
    TextureHandle texture, Color color, const Vec2i &position, const Vec2i &size, const Vec2 &anchor, float rotation) {
    draw_texture_uv_ex(texture, Vec2{0.0f, 0.0f}, Vec2{1.0f, 1.0f}, color, position, size, anchor, rotation);
}

void RenderQueueBuilder::draw_texture_uv_ex(
    TextureHandle texture, const Vec2 &uv_min, const Vec2 &uv_max, Color color, const Vec2i &position, const Vec2i &size, const Vec2 &anchor, float rotation) {
    m_queue.get().emplace_back(RenderCommandDraw{
        .texture = texture,
        .pos = Vec2{position},
        .size = Vec2{size},
        .anchor = anchor,
        .uv_min = uv_min,
        .uv_max = uv_max,
        .rotation = rotation,
        .color = color,
        // .zsort = 0,
    });
}

void RenderQueueBuilder::draw_texture_uv(TextureHandle texture, const Vec2 &uv_min, const Vec2 &uv_max, const Vec2i &position, const Vec2i &size) {
    draw_texture_uv_ex(texture, uv_min, uv_max, Color::WHITE, position, size, {0.0f, 0.0f}, 0.0f);
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
