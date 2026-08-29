#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include <glad/gl.h>
#include <glm/glm.hpp>

#include "gl_shader.h"
#include "gl_texture.h"

class GLBatchBuilder {
public:
    void init(size_t quad_count = 1000);
    void close();

    void draw_rect(float x, float y, float width, float height, const glm::vec4 &color, const GLTexture &texture, GLShader &shader);
    void flush();

private:
    struct BatchKey {
        GLuint texture;
        GLuint shader;

        bool operator==(const BatchKey &rhs) const {
            return rhs.texture == texture &&
                   rhs.shader == shader;
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

private:
    [[nodiscard]]
    bool can_fit_quad() const;

    void set_batch_key(const BatchKey &key);

private:
    GLuint m_vao, m_vbo, m_ibo;

    std::vector<BatchVertex> m_vertices;
    std::vector<std::uint32_t> m_indices;

    std::optional<BatchKey> m_current_key;
};
