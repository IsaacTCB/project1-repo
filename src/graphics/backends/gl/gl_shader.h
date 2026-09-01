#pragma once

#include <glad/gl.h>
#include <glm/fwd.hpp>

#include <string>
#include <string_view>
#include <unordered_map>

enum class ShaderType : uint8_t {
    VERTEX,
    FRAGMENT,
};

class GL_Shader {
public:
    GL_Shader(std::string_view vertex_source, std::string_view fragment_source);
    ~GL_Shader();

    bool valid() const { return m_program != 0; }
    operator bool() const { return valid(); }

    [[nodiscard]]
    GLint get_uniform(std::string_view uniform_name);

    [[nodiscard]]
    GLuint get_program() const { return m_program; }

public:
    void set_int(std::string_view uniform_name, int value);
    void set_float(std::string_view uniform_name, float value);
    void set_float2(std::string_view uniform_name, const glm::vec2 &vec);
    void set_float3(std::string_view uniform_name, const glm::vec3 &vec);
    void set_float4(std::string_view uniform_name, const glm::vec4 &vec);
    void set_mat4(std::string_view uniform_name, const glm::mat4 &mat4);

private:
    GLuint m_program;
    std::unordered_map<std::string, GLint> m_uniform_map;
};
