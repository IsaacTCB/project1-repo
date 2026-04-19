#pragma once

#include <glad/gl.h>
#include <string_view>

enum class ShaderType : uint8_t {
    Vertex,
    Fragment,
};

class GLShader {
public:
    GLShader(std::string_view vertex_source, std::string_view fragment_source);
    ~GLShader();

    [[nodiscard]]
    GLuint get_program() const { return m_program; }

private:
    GLuint m_program;
};
