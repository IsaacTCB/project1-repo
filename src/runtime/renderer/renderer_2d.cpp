#include "renderer_2d.h"

#include <variant>

#include "core/assertion.h"
#include "core/math/vec2.h"
#include "core/math/vec2i.h"
#include "core/view_context_2d.h"
#include "graphics/api/render_device.h"
#include "graphics/api/render_device_factory.h"
#include "runtime/renderer/render_handles.h"
#include "runtime/renderer/render_queue.h"

#define ASSERT_DEVICE_READY() \
    ASSERT(m_rd, "Renderer: Called renderer method without having render device initialized with 'build_device()'")

Renderer2D::Renderer2D() = default;
Renderer2D::~Renderer2D() {
    if (m_rd) {
        close_device();
    }
}

bool Renderer2D::attach_device(std::unique_ptr<IRenderDeviceFactory> factory) {
    m_rd = (*factory)();
    return m_rd != nullptr;
}

void Renderer2D::init() {
    ASSERT_DEVICE_READY();
    init_device();
}

bool Renderer2D::init_device() {
    /* create projection & view */
    m_matrices.projection = m_rd->create_matrix();
    m_matrices.view = m_rd->create_matrix();

    /* initialize batching */
    m_batch.emplace(m_rd.get());
    m_batch->init();

    /* initialize screen size */
    const Vec2i initial_size = m_rd->get_screen_size();
    on_screen_resized(initial_size);

    return true;
}

void Renderer2D::close_device() {
    ASSERT_DEVICE_READY();

    m_batch.reset();

    /* remove projection & view */
    m_rd->destroy_matrix(m_matrices.projection);
    m_rd->destroy_matrix(m_matrices.view);

    m_rd.reset();
}

/* ================== *
 *  Frame rendering   *
 * ================== */

template <class... Ts>
struct Overload : Ts... {
    using Ts::operator()...;
};

void Renderer2D::update_view_projection(const ViewContext2D &view) {
    /* update projection */
    auto ortho = view.get_ortho();
    m_rd->matrix_ortho(m_matrices.projection,
        ortho.tl.x, ortho.br.x, ortho.br.y, ortho.tl.y, -1.0f, 1.0f);

    /* update view matrix */
    m_rd->matrix_translate_rotate_scale(m_matrices.view,
        /* position */
        {view.view_camera->position.x, view.view_camera->position.y, 0.0f},

        /* rotation */
        view.view_camera->rotation,
        {0.0f, 0.0f, 1.0f},

        /* scale */
        {1.0f, 1.0f, 1.0f},

        /* offset */
        {view.view_camera->offset.x, view.view_camera->offset.y, 0.0f});
}

void Renderer2D::render_frame(const RenderContext &ctx) {
    ASSERT_DEVICE_READY();

    m_rd->begin_frame();

    m_rd->clear(ctx.clear_color);
    m_rd->apply_viewport(m_screen_viewport, ctx.frame.viewport);

    update_view_projection(*ctx.view_context);
    m_rd->set_mvp_matrix({
        .projection = m_matrices.projection,
        .view = m_matrices.view,
    });

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

TextureHandle Renderer2D::load_texture(ImageData &data) {
    ASSERT_DEVICE_READY();
    /* TODO: use a stable pointer instead */
    return m_rd->load_texture(data);
}

void Renderer2D::destroy_texture(TextureHandle texture) {
    ASSERT_DEVICE_READY();
    m_rd->destroy_texture(texture);
}

/* ================== *
 *       Other        *
 * ================== */

void Renderer2D::on_screen_resized(const Vec2i &screen_size) {
    ASSERT_DEVICE_READY();
    ASSERT(screen_size.x > 0 && screen_size.y > 0, "Width & height must not be 0.");
    m_screen_viewport = m_rd->build_viewport(screen_size);
}
