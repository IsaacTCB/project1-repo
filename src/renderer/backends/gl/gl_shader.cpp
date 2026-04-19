#include "gl_shader.h"

#include <core/logger.h>
#include <core/assertion.h>

#include <string_view>
#include <vector>

/**
 * @brief Compiles GLSL sources into a shader object
 *
 * @param shader_source The specific shader source
 * @param shader_type What kind of shader is this? (GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, etc.)
 * @return The glShader object number or 0 on fail.
 */
static GLuint compile_shader(const char* shader_source, GLenum shader_type);

GLShader::GLShader(std::string_view vertex_source, std::string_view fragment_source) {
    m_program = glCreateProgram();
    ASSERT(m_program != 0);

    auto vertex_shader = compile_shader(vertex_source.data(), GL_VERTEX_SHADER);
    glAttachShader(m_program, vertex_shader);
    auto fragment_shader = compile_shader(fragment_source.data(), GL_FRAGMENT_SHADER); 
    glAttachShader(m_program, fragment_shader);
    glLinkProgram(m_program);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    int success = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success) {
        int length = 0;
        glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> error_msg(length);
        glGetProgramInfoLog(m_program, length, nullptr, error_msg.data());
        LOG_ERROR("SHADER LINK ERROR: {0}", error_msg.data());
        ASSERT(false);
    }
}

GLShader::~GLShader() {
    ASSERT(m_program != 0);
    glDeleteProgram(m_program);
}

static std::string_view get_shader_type(GLenum shader_type) {
    switch (shader_type) {
        case GL_VERTEX_SHADER:
            return "VERTEX";
        case GL_FRAGMENT_SHADER:
            return "FRAGMENT";
        default:
            return "UNKNOWN";
    }
}

static GLuint compile_shader(const char* shader_source, GLenum shader_type) {
    GLint shader;
    shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, &shader_source, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        int length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> error_msg(length);
        glGetShaderInfoLog(shader, length, nullptr, error_msg.data());
        LOG_ERROR("{0} SHADER COMPILE ERROR: {1}",
                     get_shader_type(shader_type),
                     error_msg.data());
        ASSERT(false);
        return 0;
    }
    return shader;
}
