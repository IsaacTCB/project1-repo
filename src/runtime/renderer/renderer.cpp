#include "renderer.h"

#include <array>
#include <cstdint>

#include "core/assertion.h"
#include "core/math/vec2.h"
#include "core/math/vec2i.h"
#include "core/math/vec3.h"
#include "graphics/api/render_device.h"
#include "graphics/api/render_device_factory.h"
#include "graphics/api/vertex_layout.h"

#define ASSERT_DEVICE_READY() \
    ASSERT(m_rd, "Renderer: Called renderer method without having render device initialized with 'build_device()'")

struct Vertex2D {
    Vec2 uv;
    uint32_t color;
    Vec3 position;
};

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
    const std::array<VertexAttribute, 3> attrib{{
        {.semantic = VertexAttribute::Semantic::POSITION, .format = VertexAttribute::Format::FLOAT3, .offset = offsetof(Vertex2D, position)},
        {.semantic = VertexAttribute::Semantic::TEXCOORD, .format = VertexAttribute::Format::FLOAT2, .offset = offsetof(Vertex2D, uv)},
        {.semantic = VertexAttribute::Semantic::COLOR, .format = VertexAttribute::Format::RGBA8888, .offset = offsetof(Vertex2D, color)},
    }};

    Vertex2D vertices[] = {
        {
            .uv = Vec2{0.0f, 0.0f},
            .color = 0xFF0000FF,
            .position = Vec3{-0.9f, -0.9f, 0.0f},
        },
        {
            .uv = Vec2{1.0f, 0.0f},
            .color = 0x00FF00FF,
            .position = Vec3{0.9f, -0.9f, 0.0f},
        },
        {
            .uv = Vec2{1.0f, 1.0f},
            .color = 0x0000FFFF,
            .position = Vec3{0.9f, 0.9f, 0.0f},
        },
        {
            .uv = Vec2{0.0f, 1.0f},
            .color = 0xFFFF00FF,
            .position = Vec3{-0.9f, 0.9f, 0.0f},
        },
    };

    uint16_t indices[] = {0, 1, 2, 2, 3, 0};

    LOG_TRACE("create pipeline buffer");
    m_pipeline = m_rd->create_pipeline({
        .index_format = IndexFormat::UINT16,
        .vertex_layout = {
            .attributes = attrib,
            .stride = sizeof(Vertex2D),
        },
    });

    LOG_TRACE("create vertex buffer");
    m_vertex_buffer = m_rd->create_buffer({
        .type = BufferType::VERTEX,
        .usage = BufferUsage::STATIC,
        .size = sizeof(vertices),
        .ptr = vertices,
    });

    LOG_TRACE("create index buffer");
    m_index_buffer = m_rd->create_buffer({
        .type = BufferType::INDEX,
        .usage = BufferUsage::DYNAMIC,
        .size = sizeof(indices),
    });
    m_rd->update_buffer(m_index_buffer, sizeof(indices), indices, 0);

    /* initialize screen size */
    const Vec2i initial_size = m_rd->get_screen_size();
    on_screen_resized(initial_size);

    return true;
}

void Renderer::close_device() {
    ASSERT_DEVICE_READY();

    m_rd->destroy_buffer(m_vertex_buffer);
    m_rd->destroy_buffer(m_index_buffer);
    m_rd->destroy_pipeline(m_pipeline);

    m_rd.reset();
}

/* ================== *
 *  Frame rendering   *
 * ================== */

void Renderer::render_pass(const RenderContext &ctx) {
    ASSERT_DEVICE_READY();

    m_rd->begin_frame();
    m_rd->clear(ctx.clear_color);

    m_rd->apply_viewport(m_screen_viewport, ctx.frame.viewport);

    m_rd->set_pipeline(m_pipeline);
    m_rd->set_vertex_buffer(m_vertex_buffer);
    m_rd->set_index_buffer(m_index_buffer);

    m_rd->draw_elements(6);

    /* TODO: process commands here */

    m_rd->end_frame();
}

/* ================== *
 *  Texture Actions   *
 * ================== */

TextureHandle Renderer::load_texture(ImageData &data) {
    ASSERT_DEVICE_READY();
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
