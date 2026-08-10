#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include <glad/gl.h>
#include <glm/glm.hpp>

#include "gl_texture.h"

class GLBatchBuilder {
public:
    void init(size_t quad_count = 1000);
    void close();

    void draw_rect(float x, float y, float width, float height, const glm::vec4 &color, const GLTexture &texture);
    void flush();

private:
    [[nodiscard]]
    bool can_fit_quad() const;

private:
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ibo;

private:
    struct BatchKey {
        GLuint texture;

        bool operator==(const BatchKey &rhs) const {
            return rhs.texture == texture;
        }

        bool operator!=(const BatchKey &rhs) const {
            return !(*this == rhs);
        }
    };

    struct BatchVertex {
        glm::vec3 position;
        glm::vec2 uv;
        glm::vec4 color;
    };

    std::vector<BatchVertex> m_vertices;
    std::vector<std::uint32_t> m_indices;

    std::optional<BatchKey> m_current_key;
};
