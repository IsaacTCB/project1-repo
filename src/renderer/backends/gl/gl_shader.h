#pragma once

#include <glad/gl.h>
#include <glm/fwd.hpp>

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
    GLuint get_uniform(std::string_view uniform_name);

    [[nodiscard]]
    GLuint get_program() const { return m_program; }

public:
    void set_int(std::string_view uniform_name, int value);
    void set_float(std::string_view uniform_name, float value);
    void set_float2(std::string_view uniform_name, const glm::vec2& vec);
    void set_float3(std::string_view uniform_name, const glm::vec3& vec);
    void set_float4(std::string_view uniform_name, const glm::vec4& vec);
    void set_mat4(std::string_view uniform_name, const glm::mat4& mat4);

private:
    GLuint m_program;
};
