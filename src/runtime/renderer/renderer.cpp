#include "renderer.h"

#include "core/assertion.h"
#include "core/math/vec2i.h"
#include "graphics/api/render_device.h"
#include "graphics/api/render_device_factory.h"

Renderer::Renderer() = default;
Renderer::~Renderer() = default;

#define ASSERT_DEVICE_READY() ASSERT(m_rd, "Called renderer function without having render device initialized")

bool Renderer::build_device(std::unique_ptr<IRenderDeviceFactory> factory) {
    m_rd = (*factory)();
    if (m_rd) {
        const Vec2i initial_size = m_rd->get_screen_size();
        on_screen_resized(initial_size);
        return true;
    }

    return false;
}

void Renderer::close_device() {
    ASSERT_DEVICE_READY();
    m_rd.reset();
}

/* render pass */

void Renderer::render_pass(const RenderContext &ctx) {
    ASSERT_DEVICE_READY();

    /* start a new frame */
    m_rd->begin_frame();

    /* prepare frame */
    m_rd->apply_viewport(m_screen_viewport, ctx.frame.viewport);
    m_rd->clear(ctx.clear_color);

    /* close frame */
    m_rd->end_frame();
}

/* texture actions */

TextureHandle Renderer::load_texture(ImageData &data) {
    ASSERT_DEVICE_READY();
    return m_rd->load_texture(data);
}

void Renderer::destroy_texture(TextureHandle texture) {
    ASSERT_DEVICE_READY();
    m_rd->destroy_texture(texture);
}

/* other */

void Renderer::on_screen_resized(const Vec2i &screen_size) {
    ASSERT_DEVICE_READY();
    ASSERT(screen_size.x > 0 && screen_size.y > 0, "Width & height must not be 0.");
    m_screen_viewport = m_rd->build_viewport(screen_size);
}
