#include "gl_render_device.h"

#include <cstddef>
#include <tuple>

#include "core/assertion.h"
#include "core/logger.h"
#include "core/viewport.h"
#include "graphics/api/render_device.h"
#include "graphics/api/vertex_layout.h"
#include "runtime/renderer/render_handles.h"

#include "gl_context.h"
#include "gl_shader_sources.h"

#include <SDL3/SDL_video.h>
#include <glad/gl.h>

static bool s_glad_loaded = false;

static bool init_glad(GLADloadfunc func) {
    int version = gladLoadGL(func);
    return (version != 0);
}

GL_RenderDevice::GL_RenderDevice(std::unique_ptr<GL_IContext> &&context)
    : m_ctx(std::move(context)) {
    m_ctx->get_load_proc();

    if (!s_glad_loaded) {
        s_glad_loaded = true;
        bool success = init_glad(
            static_cast<GLADloadfunc>(m_ctx->get_load_proc()));
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
}

GL_RenderDevice::~GL_RenderDevice() = default;

/* ================= *
 *      Frames       *
 * ================= */

void GL_RenderDevice::begin_frame() {
    /* TODO: prepare view proj matrix */
}

void GL_RenderDevice::end_frame() {
    /* TODO: flush */
    m_ctx->swap_buffers();
}

/* ================= *
 *     Pipeline      *
 * ================= */

std::tuple<GLenum, GLint> get_gl_attrib_type(VertexAttribute::Format format) {
    switch (format) {
        case VertexAttribute::Format::FLOAT:
            return {GL_FLOAT, 1};
        case VertexAttribute::Format::FLOAT2:
            return {GL_FLOAT, 2};
        case VertexAttribute::Format::FLOAT3:
            return {GL_FLOAT, 3};
        case VertexAttribute::Format::FLOAT4:
            return {GL_FLOAT, 4};
        default:
            ASSERT(false, "Unsupported GL format!");
            return {GL_FLOAT, 0};
    }
}

static void bind_vertex_layout(const VertexLayout &layout) {
    const auto stride = static_cast<GLuint>(layout.stride);

    GLuint idx = 0;
    for (const VertexAttribute &attrib : layout.attributes) {
        const auto [size, type] = get_gl_attrib_type(attrib.format);

        /* unrecognized format, skip */
        if (size == 0) {
            continue;
        }

        /* apply attribute */
        const auto *offset = reinterpret_cast<void *>(attrib.offset);
        glVertexAttribPointer(idx, size, type, GL_FALSE, stride, offset);
        glEnableVertexAttribArray(idx);

        idx++;
    }
}

PipelineHandle GL_RenderDevice::create_pipeline(const PipelineDesc &pipeline_desc) {
    GL_PipelineStorage storage{};

    glGenVertexArrays(1, &storage.vertex_array);
    if (!storage.vertex_array) {
        return PipelineHandle{};
    }

    glBindVertexArray(storage.vertex_array);
    bind_vertex_layout(pipeline_desc.vertex_layout);

    /* HACK: lazy-loading the shader is stupid */
    if (!m_global_shader.has_value()) {
        m_global_shader.emplace(g_vertex_shader_source, g_fragment_shader_source);
    }

    storage.shader = &(*m_global_shader);
    glUseProgram(storage.shader->get_program());

    return m_pipelines.emplace(storage);
};

void GL_RenderDevice::destroy_pipeline(PipelineHandle pipeline) {
    auto storage = m_pipelines.get_and_destroy(pipeline);
    glDeleteVertexArrays(1, &storage->vertex_array);
}

/* ================= *
 *       Buffer      *
 * ================= */

BufferHandle GL_RenderDevice::create_buffer(const BufferInitDesc &buffer_desc) {
    GL_BufferStorage storage{};
    storage.size = static_cast<GLsizeiptr>(buffer_desc.size),

    storage.format = (buffer_desc.type == BufferType::VERTEX)
                       ? GL_ARRAY_BUFFER
                       : GL_ELEMENT_ARRAY_BUFFER;

    switch (buffer_desc.usage) {
        case BufferUsage::STATIC:
            storage.usage = GL_STATIC_DRAW;
            break;
        case BufferUsage::DYNAMIC:
        case BufferUsage::STREAMED:
            storage.usage = GL_DYNAMIC_DRAW;
            break;
        default:
            ASSERT(false, "Unsupported BufferUsage value!");
            return BufferHandle{};
    }

    /* create buffer */
    storage.buffer = {};
    glGenBuffers(1, &storage.buffer);
    if (!storage.buffer) {
        return BufferHandle{};
    }

    /* prepare buffer */
    glBindBuffer(storage.format, storage.buffer);
    glBufferData(storage.format, storage.size, buffer_desc.ptr, storage.usage);

    return m_buffers.emplace(storage);
}

void GL_RenderDevice::update_buffer(BufferHandle buffer, size_t size, void *data, size_t offset) {
    const GL_BufferStorage *storage = m_buffers.get(buffer);
    if (!storage) {
        return;
    }

    ASSERT(static_cast<GLsizeiptr>(size) <= storage->size, "GL_RenderDevice::update_buffer(): subbuffer size is larger than buffer's total capacity!");

    glBindBuffer(storage->format, storage->buffer);
    glBufferSubData(
        storage->format,
        static_cast<GLsizeiptr>(offset),
        static_cast<GLsizeiptr>(size),
        data);
}

void GL_RenderDevice::destroy_buffer(BufferHandle buffer) {
    auto storage = m_buffers.get_and_destroy(buffer);
    glDeleteBuffers(1, &storage->buffer);
}

/* ================= *
 *      Texture      *
 * ================= */

TextureHandle GL_RenderDevice::load_texture([[maybe_unused]] const ImageData &data) {
    return TextureHandle{};
}

void GL_RenderDevice::destroy_texture([[maybe_unused]] TextureHandle texture) {
    /* do not do anything */
}

bool GL_RenderDevice::has_texture([[maybe_unused]] TextureHandle texture) {
    /* do not do anything */
    return false;
}

/* ================= *
 *     Commands      *
 * ================= */

void GL_RenderDevice::clear(const Color &color) {
    const auto [r, g, b, a] = color.normalized();
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

Viewport GL_RenderDevice::build_viewport(const Vec2i &size) {
    return Viewport{
        .position{},
        .size{size},
    };
}

void GL_RenderDevice::apply_viewport(const Viewport &screen_viewport, const Viewport &window_viewport) {
    /* TODO: Y flipping is kinda annoying */
    glViewport(
        static_cast<GLint>(window_viewport.position.x),
        static_cast<GLint>(
            /* flips the Y axis cuz gl... */
            screen_viewport.size.y - (window_viewport.position.y + window_viewport.size.y)),
        static_cast<GLint>(window_viewport.size.x),
        static_cast<GLint>(window_viewport.size.y));
}

Vec2i GL_RenderDevice::get_screen_size() {
    return m_ctx->get_screen_size();
}

/* ================= *
 *       Utils       *
 * ================= */

void GL_RenderDevice::print_info() {
    const char *vendor = (const char *)glGetString(GL_VENDOR);
    LOG_INFO("gl: GPU Vendor: {}", vendor);

    const char *renderer = (const char *)glGetString(GL_RENDERER);
    LOG_INFO("gl: GPU Renderer: {}", renderer);

    const char *version = (const char *)glGetString(GL_VERSION);
    LOG_INFO("gl: OpenGL Version: {}", version);

    const char *glsl_version = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
    LOG_INFO("gl: GLSL Version: {}", glsl_version);
}
