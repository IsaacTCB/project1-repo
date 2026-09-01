#include "gl_shader.h"

#include <core/assertion.h>
#include <core/logger.h>

#include <initializer_list>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

struct GL_ShaderSourceEntry {
    GLenum type;
    std::string_view source;
};

/**
 * @brief Compiles and link a shader program from source entries.
 *
 * program = build_shader_program({
 *      {.type = GL_VERTEX_SHADER, .source = vertex_source},
 *      {.type = GL_FRAGMENT_SHADER, .source = fragment_source},
 *  });
 *
 * @param sources List of shader sources to compile and link from.
 * @return A usable gl shader program or 0 on failure.
 */
static GLuint build_shader_program(std::span<const GL_ShaderSourceEntry> shaders);
static GLuint build_shader_program(std::initializer_list<const GL_ShaderSourceEntry> list);

GL_Shader::GL_Shader(std::string_view vertex_source, std::string_view fragment_source) {
    m_program = build_shader_program({
        {.type = GL_VERTEX_SHADER, .source = vertex_source},
        {.type = GL_FRAGMENT_SHADER, .source = fragment_source},
    });
    ASSERT(m_program != 0, "Failed to build shader program");
}

GL_Shader::~GL_Shader() {
    if (m_program) {
        glDeleteProgram(m_program);
    }
}

GLint GL_Shader::get_uniform(std::string_view uniform_name) {
    std::string temp(uniform_name);

    GLint uniform{};
    if (auto it = m_uniform_map.find(temp); it != m_uniform_map.end()) {
        /* found it, total cache victory */
        uniform = it->second;
    } else {
        /* cache miss, find location */
        glUseProgram(m_program);
        uniform = glGetUniformLocation(m_program, temp.c_str());

        /* add it to cache */
        auto [_, success] = m_uniform_map.try_emplace(temp, uniform);
        ASSERT(success, "Failed to insert uniform location in shader cache");
    }

    return uniform;
}

void GL_Shader::set_int(std::string_view uniform_name, int value) {
    GLint uniform = get_uniform(uniform_name);
    glUniform1i(uniform, value);
}

void GL_Shader::set_float(std::string_view uniform_name, float value) {
    GLint uniform = get_uniform(uniform_name);
    glUniform1f(uniform, value);
}

void GL_Shader::set_float2(std::string_view uniform_name, const glm::vec2 &vec) {
    GLint uniform = get_uniform(uniform_name);
    glUniform2fv(uniform, 1, glm::value_ptr(vec));
}

void GL_Shader::set_float3(std::string_view uniform_name, const glm::vec3 &vec) {
    GLint uniform = get_uniform(uniform_name);
    glUniform3fv(uniform, 1, glm::value_ptr(vec));
}

void GL_Shader::set_float4(std::string_view uniform_name, const glm::vec4 &vec) {
    GLint uniform = get_uniform(uniform_name);
    glUniform4fv(uniform, 1, glm::value_ptr(vec));
}

void GL_Shader::set_mat4(std::string_view uniform_name, const glm::mat4 &mat4) {
    GLint uniform = get_uniform(uniform_name);
    glUniformMatrix4fv(uniform, 1, GL_FALSE, glm::value_ptr(mat4));
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

/* ================= *
 *       Shader      *
 * ================= */

/**
 * @brief Compiles GLSL sources into a shader object
 *
 * @param shader_source The specific shader source
 * @param shader_type What kind of shader is this? (GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, etc.)
 * @return The glShader object number or 0 on fail.
 */
static GLuint compile_shader(const char *shader_source, GLenum shader_type) {
    GLint shader = glCreateShader(shader_type);

    glShaderSource(shader, 1, &shader_source, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        /* print error log */
        int length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> error_msg(length);
        glGetShaderInfoLog(shader, length, nullptr, error_msg.data());
        LOG_ERROR("gl: {0} SHADER COMPILE ERROR: {1}",
            get_shader_type(shader_type),
            error_msg.data());

        /* free up shader if an error happened */
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

static GLuint build_shader_program(const std::span<const GL_ShaderSourceEntry> shaders) {
    GLuint program = glCreateProgram();
    if (!program) {
        return 0;
    }

    /* compile every shader source */
    for (const auto &shd : shaders) {
        auto shader = compile_shader(shd.source.data(), shd.type);

        /* fail-safe: if compilation has failed, then return */
        if (!shader) {
            glDeleteProgram(program);
            return 0;
        }

        glAttachShader(program, shader);
        glDeleteShader(shader);
    }

    glLinkProgram(program);

    int success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {
        /* print error log */
        int length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> error_msg(length);
        glGetProgramInfoLog(program, length, nullptr, error_msg.data());
        LOG_ERROR("gl: SHADER LINK ERROR: {0}", error_msg.data());

        /* free program on failure to avoid memleaks */
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

static GLuint build_shader_program(const std::initializer_list<const GL_ShaderSourceEntry> list) {
    return build_shader_program(std::span<const GL_ShaderSourceEntry>{list.begin(), list.end()});
}
