#include "gl_batch_builder.h"

#include <array>
#include <cstddef>
#include <cstdint>

#include <glad/gl.h>
#include <glm/glm.hpp>

#include "core/assertion.h"
#include "renderer/backends/gl/gl_texture.h"

constexpr size_t QUAD_VERTEX_COUNT = 4;
constexpr size_t QUAD_INDEX_COUNT = 6;

void GLBatchBuilder::init(size_t quad_count) {
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    /* generate vertex buffer */
    m_vertices.reserve(quad_count * QUAD_VERTEX_COUNT);
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.capacity() * sizeof(BatchVertex), nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BatchVertex), (void *)offsetof(BatchVertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(BatchVertex), (void *)offsetof(BatchVertex, uv));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(BatchVertex), (void *)offsetof(BatchVertex, color));
    glEnableVertexAttribArray(2);

    /* generate index buffer */
    m_indices.reserve(quad_count * QUAD_INDEX_COUNT);
    glGenBuffers(1, &m_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);

    /* FIXME: the batch builder currently does not create a shader program for it to use.
     * That's currently being delegated to GLRenderer, but that's a hack and not a proper
     * solution. */

    /* NOTE: this batcher is currently only used for quads. In the future we might
     * want to add other primitive shapes, like triangles and shader-less circles.
     * That's why we are building the index buffer dynamically per-frame. */
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.capacity() * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);
}

void GLBatchBuilder::close() {
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ibo);
    glDeleteVertexArrays(1, &m_vao);

    m_vertices.clear();
    m_indices.clear();
}

bool GLBatchBuilder::can_fit_quad() const {
    return m_vertices.size() <= m_vertices.capacity() - QUAD_VERTEX_COUNT &&
           m_indices.size() <= m_indices.capacity() - QUAD_INDEX_COUNT;
}

void GLBatchBuilder::draw_rect(float x, float y, float width, float height, const glm::vec4 &color, const GLTexture &texture) {
    if (!can_fit_quad()) {
        flush();
    }

    /* build BatchKey */
    BatchKey key{
        .texture = texture.id,
    };

    if (!m_current_key.has_value()) {
        m_current_key.emplace(key);
    }

    if (*m_current_key != key) {
        flush();
        m_current_key.emplace(key);
    }

    const auto index_base =
        static_cast<std::uint32_t>(m_vertices.size());

    m_vertices.emplace_back(BatchVertex{
        .position{x, y, 0.0f},
        .uv{0.0f, 0.0f},
        .color{color},
    });
    m_vertices.emplace_back(BatchVertex{
        .position{x + width, y, 0.0f},
        .uv{1.0f, 0.0f},
        .color{color},
    });
    m_vertices.emplace_back(BatchVertex{
        .position{x + width, y + height, 0.0f},
        .uv{1.0f, 1.0f},
        .color{color},
    });
    m_vertices.emplace_back(BatchVertex{
        .position{x, y + height, 0.0f},
        .uv{0.0f, 1.0f},
        .color{color},
    });

    /* load index buffer offsets */
    constexpr std::array<std::uint32_t, 6> INDEX_OFFSETS{{
        // clang-format off
        0, 1, 2,
        2, 3, 0,
        // clang-format on
    }};
    for (const auto offset : INDEX_OFFSETS) {
        m_indices.emplace_back(index_base + offset);
    }
}

void GLBatchBuilder::flush() {
    /* fail-safe: nothing was drawn, don't flush. */
    if (m_vertices.size() == 0) {
        m_current_key.reset();
        return;
    }

    if (!m_current_key.has_value()) {
        ASSERT(false, "GLBatchBuilder::flush(): Attempted to flush without having set a key beforehand.");
        return;
    }

    glBindVertexArray(m_vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_current_key.has_value() ? m_current_key->texture : 1);

    /* copy vertex buffer */
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(
        GL_ARRAY_BUFFER, 0,
        m_vertices.size() * sizeof(BatchVertex),
        static_cast<void *>(m_vertices.data()));

    /* copy index buffer */
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferSubData(
        GL_ELEMENT_ARRAY_BUFFER, 0,
        m_indices.size() * sizeof(std::uint32_t),
        static_cast<void *>(m_indices.data()));

    glDrawElements(
        GL_TRIANGLES,
        static_cast<GLsizei>(m_indices.size()),
        GL_UNSIGNED_INT,
        nullptr);

    m_vertices.clear();
    m_indices.clear();
    m_current_key.reset();
}
