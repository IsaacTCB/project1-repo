#include "batch_builder_2d.h"

#include <array>
#include <cmath>
#include <cstddef>

#include "core/assertion.h"
#include "core/math/base.h"
#include "graphics/api/render_device.h"

constexpr size_t QUAD_VERTEX_COUNT = 4;
constexpr size_t QUAD_INDEX_COUNT = 6;

void BatchBuilder2D::init(size_t quad_count) {
    const std::array<VertexAttribute, 3> attrib = {{
        {.semantic = VertexAttribute::Semantic::POSITION, .format = VertexAttribute::Format::FLOAT3, .offset = offsetof(BatchVertex2D, position)},
        {.semantic = VertexAttribute::Semantic::TEXCOORD, .format = VertexAttribute::Format::FLOAT2, .offset = offsetof(BatchVertex2D, uv)},
        {.semantic = VertexAttribute::Semantic::COLOR, .format = VertexAttribute::Format::RGBA8888, .offset = offsetof(BatchVertex2D, color)},
    }};

    m_state.pipeline = m_rd->create_pipeline({
        .index_format = IndexFormat::UINT16,
        .vertex_layout = {
            .attributes = attrib,
            .stride = sizeof(BatchVertex2D),
        },
    });

    m_state.samplers = {
        .nearest = m_rd->create_sampler({
            .filter = {
                .mag = TextureFilter::NEAREST,
                .min = TextureFilter::NEAREST,
            },
        }),
        .linear = m_rd->create_sampler({
            .filter = {
                .mag = TextureFilter::LINEAR,
                .min = TextureFilter::LINEAR,
            },
        }),
    };

    /* generate vertex buffer */
    m_vertices.reserve(quad_count * QUAD_VERTEX_COUNT);
    m_state.vertex_buffer = m_rd->create_buffer({
        .type = BufferType::VERTEX,
        .usage = BufferUsage::STREAMED,
        .size = m_vertices.capacity() * sizeof(BatchVertex2D),
    });

    /* NOTE: this batcher is currently only used for quads. In the future we might
     * want to add other primitive shapes, like triangles and shader-less circles.
     * That's why we are building the index buffer dynamically per-frame. */
    m_indices.reserve(quad_count * QUAD_INDEX_COUNT);
    m_state.index_buffer = m_rd->create_buffer({
        .type = BufferType::INDEX,
        .usage = BufferUsage::STREAMED,
        .size = m_indices.capacity() * sizeof(uint16_t),
    });
}

void BatchBuilder2D::close() {
    /* cleanup buffers */
    m_rd->destroy_buffer(m_state.index_buffer);
    m_rd->destroy_buffer(m_state.vertex_buffer);

    /* delete sampler */
    m_rd->destroy_sampler(m_state.samplers.linear);
    m_rd->destroy_sampler(m_state.samplers.nearest);

    m_rd->destroy_pipeline(m_state.pipeline);

    m_vertices.clear();
    m_indices.clear();
}

bool BatchBuilder2D::can_fit_quad() const {
    return m_vertices.size() <= m_vertices.capacity() - QUAD_VERTEX_COUNT &&
           m_indices.size() <= m_indices.capacity() - QUAD_INDEX_COUNT;
}

void BatchBuilder2D::set_batch_key(const BatchKey &key) {
    /* if the no key has been set, just set it and skip */
    if (!m_current_key.has_value()) {
        m_current_key.emplace(key);
        return;
    }

    /* if key is the same, don't flush */
    if (*m_current_key == key) {
        return;
    }

    /* the new key is different, flush all quads into a draw call */
    flush();
    m_current_key.emplace(key);
}

void BatchBuilder2D::draw_rect(const Vec2 &pos, const Vec2 &size, const Vec2 &anchor, float rotation, Color color, TextureHandle texture) {
    if (!can_fit_quad()) {
        flush();
    }

    set_batch_key({
        .texture = texture,
        .sampler = m_state.samplers.linear,
    });

    const auto index_base =
        static_cast<std::uint32_t>(m_vertices.size());

    constexpr std::array<Vec2, 4> CORNERS{{
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f},
    }};

    if (is_zero_approx(rotation)) [[likely]] {
        /* not rotated, skip slow path */
        const Vec2 offset = pos - anchor * size;
        for (const auto &corner : CORNERS) {
            Vec2 position = offset + corner * size;
            m_vertices.emplace_back(BatchVertex2D{
                .uv = corner,
                .color = color,
                .position = {position.x, position.y, 0.0f},
            });
        }
    } else {
        const float c = std::cos(rotation);
        const float s = std::sin(rotation);

        const Vec2 anchor_offset = anchor * size;
        for (const auto &corner : CORNERS) {
            const Vec2 local = (corner * size) - anchor_offset;
            const Vec2 position{
                pos.x + ((local.x * c) - (local.y * s)),
                pos.y + ((local.x * s) + (local.y * c)),
            };

            m_vertices.emplace_back(BatchVertex2D{
                .uv = corner,
                .color = color,
                .position = {position.x, position.y, 0.0f},
            });
        }
    }

    push_quad_indices(index_base);
}

void BatchBuilder2D::flush() {
    /* fail-safe: nothing was drawn, don't flush. */
    if (m_vertices.size() == 0) {
        m_current_key.reset();
        return;
    }

    if (!m_current_key.has_value()) {
        ASSERT(false, "BatchBuilder::flush(): Attempted to flush without having set a key beforehand.");
        return;
    }

    m_rd->set_pipeline(m_state.pipeline);

    /* update vertex buffer */
    m_rd->update_buffer(m_state.vertex_buffer, m_vertices.size() * sizeof(BatchVertex2D), m_vertices.data());
    m_rd->set_vertex_buffer(m_state.vertex_buffer);

    /* update index buffer */
    m_rd->update_buffer(m_state.index_buffer, m_indices.size() * sizeof(uint16_t), m_indices.data());
    m_rd->set_index_buffer(m_state.index_buffer);

    /* use texture */
    m_rd->set_texture(0, m_current_key->texture, m_current_key->sampler);

    m_rd->draw_elements(m_indices.size());

    m_vertices.clear();
    m_indices.clear();
    m_current_key.reset();
}

void BatchBuilder2D::push_quad_indices(size_t index_base) {
    /* load index buffer offsets */
    using IndexType = std::remove_reference_t<decltype(m_indices[0])>;
    constexpr std::array<IndexType, 6> INDEX_OFFSETS{{
        // clang-format off
        0, 1, 2,
        2, 3, 0,
        // clang-format on
    }};
    for (const auto offset : INDEX_OFFSETS) {
        const auto index = static_cast<IndexType>(index_base + offset);
        m_indices.emplace_back(index);
    }
}
