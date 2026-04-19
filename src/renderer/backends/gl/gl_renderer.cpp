#include "gl_renderer.h"
#include "gl_context.h"
#include "gl_shader.h"

#include "core/logger.h"

#include <glad/gl.h>

static bool s_glad_loaded = false;

struct ImplTexture {
    GLuint id;
    size_t width;
    size_t height;
};

struct GLRenderer::Impl {
    GLContext context;
    HandlePool<ImplTexture> texture_pool;
    std::unique_ptr<GLShader> shader;
};

static bool init_glad(GLADloadfunc func) {
    int version = gladLoadGL(func);
    return (version != 0);
}

GLRenderer::GLRenderer(DisplayWindowHandle& window_data)
    :m_impl(std::make_unique<Impl>()) {
    m_impl->context.init(window_data.raw_window_handle);
    if (!s_glad_loaded) {
        s_glad_loaded = true;
        bool success = init_glad(
            static_cast<GLADloadfunc>(m_impl->context.load_func)
        );
        if (!success) {
            LOG_CRITICAL("Glad failed to load OpenGL.");
        }
    }

    const char* vertex_shader_source = R"(
        #version 330 core

        layout (location = 0) in vec3 a_pos;
        layout (location = 1) in vec2 a_uv;

        out vec2 v_uv;

        void main() {
            v_uv = a_uv;
            gl_Position = vec4(a_pos, 1.0);
        }
    )";

    const char* fragment_shader_source = R"(
        #version 330 core

        in vec2 v_uv;

        out vec4 color;

        uniform sampler2D u_texture;

        void main() {
            color = texture(u_texture, v_uv);
        }
    )";
    m_impl->shader = std::make_unique<GLShader>(vertex_shader_source, fragment_shader_source);
}

GLRenderer::~GLRenderer() = default;

void GLRenderer::clear(float r, float g, float b, float a) {
    m_impl->context.bind();
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void GLRenderer::swap_buffers() {
    m_impl->context.swap_buffers();
}

TextureHandle GLRenderer::load_texture(ImageData& image) {
    ASSERT(image.pixels.size() > 0, "Image is not loaded");

    GLenum internal_format, data_format;
    switch (image.channels) {
        case 3:
            internal_format = GL_RGB8;
            data_format = GL_RGB;
            break;
        case 4:
            internal_format = GL_RGBA8;
            data_format = GL_RGBA;
            break;
        default:
            ASSERT(false, "Unsupported bpp");
            break;
    }

    GLuint texture_id;
    glGenTextures(1, &texture_id);
    ASSERT(texture_id != 0, "Unable to create gl texture");

    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(
        GL_TEXTURE_2D, 0, internal_format,
        image.width, image.height, 0,
        data_format, GL_UNSIGNED_BYTE,
        image.pixels.data());

    glBindTexture(GL_TEXTURE_2D, 0);

    ImplTexture texture{
        .id = texture_id,
        .width = image.width,
        .height = image.height,
    };
    LOG_INFO("texture is {0}", texture.id);
    return m_impl->texture_pool.emplace(texture);
}

void GLRenderer::destroy_texture(TextureHandle handle) {
    ImplTexture* texture = m_impl->texture_pool.get(handle);
    glDeleteTextures(1, &texture->id);
    texture->id = 0;
    m_impl->texture_pool.remove(handle);
}

void GLRenderer::draw_texture(TextureHandle handle, int x, int y, int width, int height) {
    ImplTexture* texture = m_impl->texture_pool.get(handle);
    glBindTexture(GL_TEXTURE_2D, texture->id);
}
