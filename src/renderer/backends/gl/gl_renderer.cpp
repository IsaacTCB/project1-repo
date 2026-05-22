#include "gl_renderer.h"
#include "gl_context.h"
#include "gl_shader.h"
#include "gl_shader_sources.h"

#include "core/camera.h"
#include "core/logger.h"
#include "core/view_context_2d.h"
#include "core/viewport.h"

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <variant>

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

    glm::mat4 proj;
    GLuint vao, vbo, ibo;

    ImplTexture white_texture;
};

static bool init_glad(GLADloadfunc func) {
    int version = gladLoadGL(func);
    return (version != 0);
}

static ImplTexture gl_create_texture(ImageData &image);

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

    glGenVertexArrays(1, &m_impl->vao);
    glBindVertexArray(m_impl->vao);

    // clang-format off
    float vertices[]{
        0.0f, 0.0f, 0.0f,  0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,  1.0f, 1.0f,
        0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
    };

    uint32_t indices[] = {
        0, 1, 2,
        2, 3, 0,
    };
    // clang-format on

    glGenBuffers(1, &m_impl->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_impl->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(sizeof(float) * 0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &m_impl->ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_impl->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    m_impl->shader = std::make_unique<GLShader>(
        g_vertex_shader_source,
        g_fragment_shader_source);

    glUseProgram(m_impl->shader->get_program());
    m_impl->shader->set_int("u_texture", 0);

    on_screen_resized(
        window_data.props.width,
        window_data.props.height);
}

GLRenderer::~GLRenderer() {
    glDeleteBuffers(1, &m_impl->ibo);
    glDeleteBuffers(1, &m_impl->vbo);
    glDeleteVertexArrays(1, &m_impl->vao);
}

void GLRenderer::clear(float r, float g, float b, float a) {
    m_impl->context.bind();
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void GLRenderer::swap_buffers() {
    m_impl->context.bind();
    m_impl->context.swap_buffers();
}

void GLRenderer::on_screen_resized(uint32_t width, uint32_t height) {
    ASSERT(width != 0 && height != 0, "Width & height must not be 0.");
    m_screen_viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(width),
        .height = static_cast<float>(height),
    };
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);
}

TextureHandle GLRenderer::load_texture(ImageData &image) {
    m_impl->context.bind();
    ASSERT(image.pixels.size() > 0, "Image is not loaded");
    ImplTexture texture = gl_create_texture(image);
    LOG_TRACE("gl: loaded texture: {0} ({1}x{2})", texture.id, image.width, image.height);
    return m_impl->texture_pool.emplace(texture);
}

void GLRenderer::destroy_texture(TextureHandle handle) {
    m_impl->context.bind();
    auto texture = m_impl->texture_pool.get_and_destroy(handle);
    glDeleteTextures(1, &texture->id);
}

static glm::mat4 get_camera_projection(Camera2D *camera, const glm::vec2 &size) {
    float left, right, top, bottom;
    if (camera->anchor == Camera2D::AnchorMode::CENTER) {
        float width_half = size.x * 0.5f;
        float height_half = size.y * 0.5f;

        left = -width_half;
        right = width_half;
        bottom = height_half; /* flip it cuz opengl... */
        top = -height_half;
    } else {
        left = 0.0f;
        right = size.x;
        bottom = size.y; /* same as above */
        top = 0.0f;
    }

    float zoom = 1.0f / std::max(camera->zoom, 0.0001f);
    left *= zoom;
    right *= zoom;
    top *= zoom;
    bottom *= zoom;

    return glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
}

static glm::mat4 get_camera_view_matrix(Camera2D *camera) {
    auto i = glm::identity<glm::mat4>();

    return glm::translate(i, {camera->x, camera->y, 0.0f}) *
           glm::rotate(i, camera->rotation, {0.0f, 0.0f, 1.0f}) *
           glm::translate(i, {camera->offset_x, camera->offset_y, 0.0f});
}

void GLRenderer::render_pass(const ViewContext2D &view, const RenderQueue &queue) {
    m_impl->context.bind();
    apply_viewport(view.window_viewport);

    glm::vec2 view_size{view.view_width, view.view_height};
    glm::mat4 proj_matrix = get_camera_projection(view.view_camera, view_size);
    glm::mat4 view_matrix = get_camera_view_matrix(view.view_camera);
    m_impl->proj = proj_matrix * glm::inverse(view_matrix);

    glUseProgram(m_impl->shader->get_program());
    m_impl->shader->set_mat4("u_view_proj", m_impl->proj);

    for (const auto &command : queue.get()) {
        std::visit([this](const auto &cmd) {
            this->command(cmd);
        },
            command);
    }
}

/* commands */

static ImplTexture gl_create_texture(ImageData &image) {
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
    glBindVertexArray(0);

    return ImplTexture{
        .id = texture_id,
        .width = image.width,
        .height = image.height,
    };
}

void GLRenderer::command(const RenderCommandDraw &command) {
    ImplTexture *texture = nullptr;
    if (command.texture.has_value()) {
        TextureHandle handle = command.texture.value();
        texture = m_impl->texture_pool.get(handle);
    } else {
        texture = &m_impl->white_texture;
    }
    ASSERT(texture, "Texture is still nullptr");

    glBindVertexArray(m_impl->vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_impl->ibo);

    glm::vec4 color;
    color.r = ((command.color & 0xFF000000) >> 24) / static_cast<float>(0xFF);
    color.g = ((command.color & 0x00FF0000) >> 16) / static_cast<float>(0xFF);
    color.b = ((command.color & 0x0000FF00) >> 8) / static_cast<float>(0xFF);
    color.a = ((command.color & 0x000000FF) >> 0) / static_cast<float>(0xFF);

    glm::mat4 model{1.0f};
    model = glm::translate(model, {command.x, command.y, 0.0f});
    model = glm::scale(model, {command.width, command.height, 1.0f});
    m_impl->shader->set_mat4("u_model", model);
    m_impl->shader->set_float4("u_color", color);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture->id);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

void GLRenderer::apply_viewport(const Viewport &viewport) {
    glViewport(
        viewport.x,
        m_screen_viewport.height - (viewport.y + viewport.height), /* flips the Y axis cuz gl... */
        viewport.width,
        viewport.height);
}
