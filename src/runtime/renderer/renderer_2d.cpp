#include "renderer_2d.h"

#include <array>
#include <cstdint>
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

    m_render_target = m_rd->create_render_target({
        .size = initial_size,
    });
    if (m_render_target) {
        m_enabled_render_targets = true;
    }

    return true;
}

void Renderer2D::close_device() {
    ASSERT_DEVICE_READY();

    m_batch.reset();

    m_rd->destroy_render_target(m_render_target);

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

    if (m_enabled_render_targets) {
        const auto render_target_size = ctx.view_context->layout.render_target_size;
        if (m_render_target_size != render_target_size) {
            m_render_target_size = render_target_size;
            LOG_TRACE(m_render_target_size);
            m_rd->resize_render_target(m_render_target, m_render_target_size);
        }
    }

    /* draw it into the render target */
    render_frame_pass(ctx);
    if (m_enabled_render_targets) {
        /* draw render target into backbuffer */
        m_rd->begin_pass({
            .target = {},
            .viewport = ctx.view_context->layout.presentation_rect,
            .clear_color = Color::BLACK,
        });
        draw_fullscreen_quad(m_rd->get_color_attachment(m_render_target));
        m_rd->end_pass();
    }

    m_rd->end_frame();
}

void Renderer2D::render_frame_pass(const RenderContext &ctx) {
    RenderPassApplyDesc pass_desc;
    if (m_enabled_render_targets) {
        /* render to render target */
        pass_desc = {
            .target = m_render_target,
            .viewport = ctx.view_context->layout.render_viewport,
            .clear_color = ctx.clear_color,
        };
    } else {
        /* if render targets are disabled, then
         * just render to the backbuffer directly */
        pass_desc = {
            .target = {},
            .viewport = ctx.view_context->layout.presentation_rect,
            .clear_color = ctx.clear_color,
        };
    }
    m_rd->begin_pass(pass_desc);

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
                    m_batch->draw_rect(draw.pos, draw.size, draw.anchor, draw.rotation, draw.color, draw.texture);
                },
            },
            command);
    }

    /* @NOTE: must flush before finishing the pass! */
    m_batch->flush();

    m_rd->end_pass();
}

struct BatchVertex2D {
    Vec2 uv;
    Color color;
    Vec3 position;
};

void Renderer2D::draw_fullscreen_quad(TextureHandle texture) {
    static bool inited = false;
    static PipelineHandle pipeline;
    static BufferHandle vertices;
    static BufferHandle indices;
    static SamplerHandle sampler;

    if (!inited) {
        /* HACK: lazy-init is REALLY stupid here... Also, most of these objects are
         * already used by the batch renderer, so we might just want to reuse those */
        const std::array<VertexAttribute, 3> attrib{{
            {.semantic = VertexAttribute::Semantic::POSITION, .format = VertexAttribute::Format::FLOAT3, .offset = offsetof(BatchVertex2D, position)},
            {.semantic = VertexAttribute::Semantic::TEXCOORD, .format = VertexAttribute::Format::FLOAT2, .offset = offsetof(BatchVertex2D, uv)},
            {.semantic = VertexAttribute::Semantic::COLOR, .format = VertexAttribute::Format::RGBA8888, .offset = offsetof(BatchVertex2D, color)},
        }};

        pipeline = m_rd->create_pipeline({
            .blending = {
                .enabled = false,
            },
            .index_format = IndexFormat::UINT8,
            .vertex_layout = {
                .attributes = attrib,
                .stride = sizeof(BatchVertex2D),
            },
        });

        static const BatchVertex2D vertex_data[] = {
            {.uv = {0.0f, 0.0f}, .color = Color{0xFFFFFFFF}, .position = {-1.0f, -1.0f, 0.0f}},
            {.uv = {1.0f, 0.0f}, .color = Color{0xFFFFFFFF}, .position = {1.0f, -1.0f, 0.0f}},
            {.uv = {1.0f, 1.0f}, .color = Color{0xFFFFFFFF}, .position = {1.0f, 1.0f, 0.0f}},
            {.uv = {0.0f, 1.0f}, .color = Color{0xFFFFFFFF}, .position = {-1.0f, 1.0f, 0.0f}},
        };
        static const uint8_t index_data[] = {
            // clang-format off
            0, 1, 2,
            2, 3, 0
            // clang-format on
        };

        vertices = m_rd->create_buffer({
            .type = BufferType::VERTEX,
            .usage = BufferUsage::STATIC,
            .size = sizeof(vertex_data),
            .ptr = vertex_data,
        });

        indices = m_rd->create_buffer({
            .type = BufferType::INDEX,
            .usage = BufferUsage::STATIC,
            .size = sizeof(index_data),
            .ptr = index_data,
        });

        sampler = m_rd->create_sampler({
            .filter = {
                .mag = TextureFilter::NEAREST,
                .min = TextureFilter::LINEAR,
            },
        });

        inited = true;
        LOG_TRACE("initialized fullscreen quad");
    }

    m_rd->set_pipeline(pipeline);
    m_rd->set_mvp_matrix({});
    m_rd->set_vertex_buffer(vertices);
    m_rd->set_index_buffer(indices);
    m_rd->set_texture(0, texture, sampler);

    m_rd->draw_elements(6);
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
