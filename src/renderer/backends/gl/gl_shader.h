#pragma once

#include <cstdint>
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
    unsigned int get_program() const { return m_program; }

private:
    unsigned int m_program;
};
