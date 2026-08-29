#include "renderer.h"

#include "core/assertion.h"
#include "core/math/vec2i.h"
#include "graphics/api/render_device.h"
#include "graphics/api/render_device_factory.h"

Renderer::Renderer() = default;
Renderer::~Renderer() = default;

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
    ASSERT(m_rd, "Attempted to close a render device while not having one attached")
    m_rd.reset();
}

/* render pass */

void Renderer::render_pass(const RenderContext &ctx) {
    ASSERT(m_rd, "Called renderer function without having render device initialized")

    /* start a new frame */
    m_rd->begin_frame();

    /* prepare frame */
    m_rd->apply_viewport(m_screen_viewport, ctx.frame.viewport);
    m_rd->clear(ctx.clear_color);

    /* close frame */
    m_rd->end_frame();
}

void Renderer::on_screen_resized(const Vec2i &screen_size) {
    ASSERT(m_rd, "Called renderer function without having render device initialized")
    ASSERT(screen_size.x > 0 && screen_size.y > 0, "Width & height must not be 0.");
    m_screen_viewport = m_rd->build_viewport(screen_size);
}
