#include "renderer.h"

#include <variant>

#include "core/assertion.h"
#include "core/math/vec2.h"
#include "core/math/vec2i.h"
#include "graphics/api/render_device.h"
#include "graphics/api/render_device_factory.h"
#include "runtime/renderer/render_handles.h"
#include "runtime/renderer/render_queue.h"

#define ASSERT_DEVICE_READY() \
    ASSERT(m_rd, "Renderer: Called renderer method without having render device initialized with 'build_device()'")

Renderer::Renderer() = default;
Renderer::~Renderer() {
    if (m_rd) {
        close_device();
    }
}

bool Renderer::build_device(std::unique_ptr<IRenderDeviceFactory> factory) {
    m_rd = (*factory)();
    if (m_rd) {
        return init_device();
    }

    return false;
}

bool Renderer::init_device() {
    /* initialize batching */
    m_batch.emplace(m_rd.get());
    m_batch->init();

    /* initialize screen size */
    const Vec2i initial_size = m_rd->get_screen_size();
    on_screen_resized(initial_size);

    return true;
}

void Renderer::close_device() {
    ASSERT_DEVICE_READY();
    m_batch.reset();
    m_rd.reset();
}

/* ================== *
 *  Frame rendering   *
 * ================== */

template <class... Ts>
struct Overload : Ts... {
    using Ts::operator()...;
};

void Renderer::render_pass(const RenderContext &ctx) {
    ASSERT_DEVICE_READY();

    m_rd->begin_frame();

    m_rd->clear(ctx.clear_color);
    m_rd->apply_viewport(m_screen_viewport, ctx.frame.viewport);

    for (const RenderCommand &command : ctx.queue.get()) {
        std::visit(
            Overload{
                /* draw 2D rect command */
                [&](const RenderCommandDraw &draw) {
                    /* TODO: solve texture handle */
                    m_batch->draw_rect(draw.pos, draw.size, draw.color, draw.texture);
                },
            },
            command);
    }

    /* @NOTE: must flush before finishing the frame! */
    m_batch->flush();

    m_rd->end_frame();
}

/* ================== *
 *  Texture Actions   *
 * ================== */

TextureHandle Renderer::load_texture(ImageData &data) {
    ASSERT_DEVICE_READY();
    /* TODO: use a stable pointer instead */
    return m_rd->load_texture(data);
}

void Renderer::destroy_texture(TextureHandle texture) {
    ASSERT_DEVICE_READY();
    m_rd->destroy_texture(texture);
}

/* ================== *
 *       Other        *
 * ================== */

void Renderer::on_screen_resized(const Vec2i &screen_size) {
    ASSERT_DEVICE_READY();
    ASSERT(screen_size.x > 0 && screen_size.y > 0, "Width & height must not be 0.");
    m_screen_viewport = m_rd->build_viewport(screen_size);
}
