#include "gl_renderer.h"

#include "gl_batch_builder.h"
#include "gl_context.h"
#include "gl_shader.h"
#include "gl_shader_sources.h"
#include "gl_texture.h"

#include <variant>

#include "core/camera.h"
#include "core/logger.h"
#include "core/utils/handle_pool.h"
#include "core/view_context_2d.h"
#include "core/viewport.h"
#include "renderer/api/render_handles.h"
#include "renderer/api/render_queue.h"

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

static bool s_glad_loaded = false;

struct GLRenderer::Impl {
    GLContext context;
    HandlePool<GLTexture, TextureHandle> texture_pool;
    std::unique_ptr<GLShader> shader;
    glm::mat4 proj;
    GLTexture white_texture;
    GLBatchBuilder batch;
};

static bool init_glad(GLADloadfunc func) {
    int version = gladLoadGL(func);
    return (version != 0);
}

static GLTexture gl_create_texture(ImageData &image);

GLRenderer::GLRenderer(DisplayWindowHandle &window_data)
    : m_impl(std::make_unique<Impl>()) {
    m_impl->context.init(window_data.raw_window_handle);
    m_impl->context.bind();

    if (!s_glad_loaded) {
        s_glad_loaded = true;
        bool success = init_glad(
            static_cast<GLADloadfunc>(m_impl->context.load_func));
        if (!success) {
            LOG_CRITICAL("gl: Glad failed to load OpenGL.");
        }

        print_info();
    }

    /* This allows gl to load images whose
     * data size are not a multiple of 4. */
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* load white texture to draw solid colors. */
    {
        ImageData white_pixel;
        white_pixel.pixels.resize(8 * 3, 0xFF);
        white_pixel.width = 2;
        white_pixel.height = 2;
        white_pixel.channels = 3;
        m_impl->white_texture = gl_create_texture(white_pixel);
    }

    m_impl->shader = std::make_unique<GLShader>(
        g_vertex_shader_source,
        g_fragment_shader_source);
    m_impl->shader->set_int("u_texture", 0);

    m_impl->batch.init();

    on_screen_resized(window_data.props.size);
}

GLRenderer::~GLRenderer() {
    m_impl->batch.close();
}

void GLRenderer::clear(Color color) {
    m_impl->context.bind();

    auto [r, g, b, a] = color.normalized();
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void GLRenderer::swap_buffers() {
    m_impl->context.bind();
    m_impl->context.swap_buffers();
}

void GLRenderer::on_screen_resized(const Vec2i &screen_size) {
    ASSERT(screen_size.x > 0 && screen_size.y > 0, "Width & height must not be 0.");
    m_screen_viewport = {
        .position = {0, 0},
        .size = {
            screen_size.x,
            screen_size.y,
        },
    };
    glViewport(0, 0, screen_size.x, screen_size.y);
    m_impl->proj = glm::ortho(0.0f, static_cast<float>(screen_size.x), static_cast<float>(screen_size.y), 0.0f, -1.0f, 1.0f);
    m_impl->shader->set_mat4("u_view_proj", m_impl->proj);
}

TextureHandle GLRenderer::load_texture(ImageData &image) {
    if (!image) {
        return TextureHandle{};
    }

    m_impl->context.bind();

    GLTexture texture = gl_create_texture(image);
    if (texture.id == 0) {
        return TextureHandle{};
    }

    LOG_TRACE("gl: loaded texture: {0} ({1}x{2})", texture.id, image.width, image.height);
    return m_impl->texture_pool.emplace(texture);
}

void GLRenderer::destroy_texture(TextureHandle handle) {
    auto texture = m_impl->texture_pool.get_and_destroy(handle);
    if (!texture) {
        ASSERT(false, "Attempted to delete a non-existent handle");
        LOG_ERROR("GLRenderer::destroy_texture(): Attempted to destroy texture with non-existent handle '{}'", handle);
        return;
    }

    m_impl->context.bind();
    glDeleteTextures(1, &texture->id);
}

static glm::mat4 get_camera_projection(const ViewContext2D &view) {
    /* This ortho is top-left origin, but OpenGL uses
     * bottom-left origin. As a result, that unflips
     * everything so that Y+ means down again. */
    auto ortho = view.get_ortho();
    return glm::ortho(ortho.tl.x, ortho.br.x, ortho.br.y, ortho.tl.y, -1.0f, 1.0f);
}

static glm::mat4 get_camera_view_matrix(Camera2D *camera) {
    auto i = glm::identity<glm::mat4>();

    return glm::translate(i, {camera->position.x, camera->position.y, 0.0f}) *
           glm::rotate(i, camera->rotation, {0.0f, 0.0f, 1.0f}) *
           glm::translate(i, {camera->offset.x, camera->offset.y, 0.0f});
}

void GLRenderer::render_pass(const ViewContext2D &view, const RenderQueue &queue) {
    m_impl->context.bind();
    apply_viewport(view.window_viewport);

    glm::mat4 proj_matrix = get_camera_projection(view);
    glm::mat4 view_matrix = get_camera_view_matrix(view.view_camera);
    m_impl->proj = proj_matrix * glm::inverse(view_matrix);

    m_impl->shader->set_mat4("u_view_proj", m_impl->proj);

    for (const auto &command : queue.get()) {
        std::visit([this](const auto &cmd) {
            this->command(cmd);
        },
            command);
    }

    m_impl->batch.flush();
}

/* commands */

static GLTexture gl_create_texture(ImageData &image) {
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
            return {};
    }

    GLuint texture_id;
    glGenTextures(1, &texture_id);
    if (texture_id == 0) {
        return {};
    }

    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(
        GL_TEXTURE_2D, 0, internal_format,
        static_cast<GLsizei>(image.width),
        static_cast<GLsizei>(image.height),
        0, data_format, GL_UNSIGNED_BYTE,
        image.pixels.data());

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);

    return GLTexture{
        .id = texture_id,
        .width = image.width,
        .height = image.height,
    };
}

void GLRenderer::command(const RenderCommandDraw &command) {
    GLTexture *texture = m_impl->texture_pool.get(command.texture);
    if (!texture) {
        texture = &m_impl->white_texture;
    }

    auto [col_r, col_g, col_b, col_a] = command.color.normalized();
    glm::vec4 color{
        col_r,
        col_g,
        col_b,
        col_a,
    };

    m_impl->batch.draw_rect(
        command.pos.x, command.pos.y,
        command.size.x, command.size.y,
        color, *texture, *m_impl->shader);
}

void GLRenderer::print_info() {
    const char *vendor = (const char *)glGetString(GL_VENDOR);
    LOG_INFO("gl: GPU Vendor: {}", vendor);

    const char *renderer = (const char *)glGetString(GL_RENDERER);
    LOG_INFO("gl: GPU Renderer: {}", renderer);

    const char *version = (const char *)glGetString(GL_VERSION);
    LOG_INFO("gl: OpenGL Version: {}", version);

    const char *glsl_version = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
    LOG_INFO("gl: GLSL Version: {}", glsl_version);
}

void GLRenderer::apply_viewport(const Viewport &viewport) {
    glViewport(
        static_cast<GLint>(viewport.position.x),
        static_cast<GLint>(
            /* flips the Y axis cuz gl... */
            m_screen_viewport.size.y - (viewport.position.y + viewport.size.y)),
        static_cast<GLint>(viewport.size.x),
        static_cast<GLint>(viewport.size.y));
}
