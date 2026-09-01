#include "gl_render_device.h"

#include <cstddef>
#include <optional>
#include <tuple>

#include "core/assertion.h"
#include "core/image/image.h"
#include "core/logger.h"
#include "core/utils/inline.h"
#include "core/viewport.h"
#include "graphics/api/render_device.h"
#include "graphics/api/vertex_layout.h"
#include "runtime/renderer/render_handles.h"

#include "gl_context.h"
#include "gl_shader_sources.h"

#include <SDL3/SDL_video.h>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

static bool s_glad_loaded = false;

static bool init_glad(GLADloadfunc func) {
    int version = gladLoadGL(func);
    return (version != 0);
}

static std::optional<GL_TextureStorage> build_texture(const ImageData &data);

static void gl_debug_callback(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar *message,
    const void *userParam) {
    /* fixes annoying unused variable warnings  */
    (void)(length);
    (void)(userParam);

    /* list of annoying driver warnings to be suppressed */
    constexpr std::array<GLuint, 1> IGNORED_IDS{{
        131185, /* [api/other] buffer will use video memory... */
    }};
    for (auto ignored : IGNORED_IDS) {
        if (id == ignored) {
            return;
        }
    }

    /* convert source enum to string */
    const char *source_str = "unk";
    switch (source) {
        case GL_DEBUG_SOURCE_API:
            source_str = "api";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            source_str = "window";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            source_str = "shader";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            source_str = "external";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            source_str = "app";
            break;
        case GL_DEBUG_SOURCE_OTHER:
            source_str = "other";
            break;
    }

    /* convert type enum to string */
    const char *type_str = "unk";
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            type_str = "error";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            type_str = "deprecated";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            type_str = "undefined behavior";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            type_str = "portability";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            type_str = "performance";
            break;
        case GL_DEBUG_TYPE_MARKER:
            type_str = "marker";
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            type_str = "push group";
            break;
        case GL_DEBUG_TYPE_POP_GROUP:
            type_str = "pop group";
            break;
        case GL_DEBUG_TYPE_OTHER:
            type_str = "other";
            break;
    }

    std::string formatted = std::format("gl: [{}/{}]: {}",
        source_str, type_str, message);
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            LOG_ERROR(formatted);
            break;

        case GL_DEBUG_SEVERITY_MEDIUM:
            LOG_WARN(formatted);
            break;

        case GL_DEBUG_SEVERITY_LOW:
            LOG_INFO(formatted);
            break;

        case GL_DEBUG_SEVERITY_NOTIFICATION:
            LOG_TRACE(formatted);
            break;
    }
}

static void init_debug() {
    GLint flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

        if (glDebugMessageCallback) {
            glDebugMessageCallback(gl_debug_callback, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        }
    }
}

GL_RenderDevice::GL_RenderDevice(std::unique_ptr<GL_IContext> &&context)
    : m_ctx(std::move(context)) {
    m_ctx->bind();
    if (!s_glad_loaded) {
        s_glad_loaded = true;
        bool success = init_glad(
            static_cast<GLADloadfunc>(m_ctx->get_load_proc()));
        if (!success) {
            LOG_CRITICAL("gl: Glad failed to load OpenGL.");
        }

        init_debug();
        print_info();
    }

    m_white_texture = *build_texture(ImageData{
        .pixels = {
            // clang-format off
            0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF,
            // clang-format on
        },
        .width = 2,
        .height = 2,
        .channels = 4,
    });

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
        case VertexAttribute::Format::INT:
            return {GL_BYTE, 1};
        case VertexAttribute::Format::INT16:
            return {GL_SHORT, 1};
        case VertexAttribute::Format::INT32:
            return {GL_INT, 1};

        case VertexAttribute::Format::UINT8:
            return {GL_UNSIGNED_BYTE, 1};
        case VertexAttribute::Format::UINT16:
        case VertexAttribute::Format::RGB5551:
        case VertexAttribute::Format::RGB565:
        case VertexAttribute::Format::RGBA4444:
            return {GL_UNSIGNED_SHORT, 1};
        case VertexAttribute::Format::UINT32:
        case VertexAttribute::Format::RGBA8888:
            return {GL_UNSIGNED_INT, 1};

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
        const auto [type, size] = get_gl_attrib_type(attrib.format);

        /* unrecognized format, skip */
        if (size == 0) {
            continue;
        }

        /* apply attribute */
        const auto *offset = reinterpret_cast<void *>(attrib.offset);

        switch (attrib.format) {
            case VertexAttribute::Format::RGB5551:
            case VertexAttribute::Format::RGB565:
            case VertexAttribute::Format::RGBA4444:
            case VertexAttribute::Format::RGBA8888:
                glVertexAttribIPointer(idx, size, type, stride, offset);
                break;
            default:
                glVertexAttribPointer(idx, size, type, GL_FALSE, stride, offset);
                break;
        }

        glEnableVertexAttribArray(idx);

        idx++;
    }
}

static GLenum get_index_format(IndexFormat index) {
    switch (index) {
        case IndexFormat::UINT8:
            return GL_UNSIGNED_BYTE;
        case IndexFormat::UINT16:
            return GL_UNSIGNED_SHORT;
        case IndexFormat::UINT32:
            return GL_UNSIGNED_INT;
        default:
            /* unsupported */
            return 0;
    }
}

PipelineHandle GL_RenderDevice::create_pipeline(const PipelineDesc &pipeline_desc) {
    GL_PipelineStorage storage{};

    glGenVertexArrays(1, &storage.vertex_array);
    if (!storage.vertex_array) {
        return PipelineHandle{};
    }

    glBindVertexArray(storage.vertex_array);

    /* copy layout */
    storage.attributes = std::vector<VertexAttribute>(
        pipeline_desc.vertex_layout.attributes.begin(),
        pipeline_desc.vertex_layout.attributes.end());
    storage.stride = pipeline_desc.vertex_layout.stride;

    /* HACK: lazy-loading the shader is stupid */
    if (!m_global_shader.has_value()) {
        LOG_TRACE("gl: loading default shader...");

        m_global_shader.emplace(g_vertex_shader_source, g_fragment_shader_source);
        ASSERT(m_global_shader->valid(), "gl: failed to load global shader");

        m_global_shader->set_int("u_texture", 0);
    }

    storage.index_format = get_index_format(pipeline_desc.index_format);
    if (!storage.index_format) {
        ASSERT(false, "gl: Unsupported index format!");
        return PipelineHandle{};
    }

    storage.shader = &(*m_global_shader);

    glBindVertexArray(0);
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

void GL_RenderDevice::update_buffer(BufferHandle buffer, size_t size, const void *data, size_t offset) {
    const GL_BufferStorage *storage = m_buffers.get(buffer);
    if (!storage) {
        return;
    }

    ASSERT(static_cast<GLsizeiptr>(size) <= storage->size, "GL_RenderDevice::update_buffer(): subbuffer size is larger than buffer's total capacity!");

    glBindBuffer(storage->format, storage->buffer);
    glBufferSubData(
        storage->format,
        static_cast<GLintptr>(offset),
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

static std::optional<GL_TextureStorage> build_texture(const ImageData &data) {
    GL_TextureStorage storage{};

    switch (data.channels) {
        case 3:
            storage.internal_format = GL_RGB8;
            storage.data_format = GL_RGB;
            break;
        case 4:
            storage.internal_format = GL_RGBA8;
            storage.data_format = GL_RGBA;
            break;
        default:
            ASSERT(false, "Unsupported bpp");
            return std::nullopt;
    }

    glGenTextures(1, &storage.texture);
    if (storage.texture == 0) {
        return std::nullopt;
    }

    glBindTexture(GL_TEXTURE_2D, storage.texture);
    glTexImage2D(
        GL_TEXTURE_2D, 0, storage.internal_format,
        static_cast<GLsizei>(data.width),
        static_cast<GLsizei>(data.height),
        0, storage.data_format, GL_UNSIGNED_BYTE,
        data.pixels.data());

    /* fixes an opengl message about this texture not having mipmaps */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return storage;
}

TextureHandle GL_RenderDevice::load_texture(const ImageData &data) {
    auto storage = build_texture(data);
    if (!storage.has_value()) {
        return TextureHandle{};
    }
    return m_textures.emplace(*storage);
}

void GL_RenderDevice::destroy_texture(TextureHandle texture) {
    auto storage = m_textures.get_and_destroy(texture);
    glDeleteTextures(1, &storage->texture);
}

bool GL_RenderDevice::has_texture(TextureHandle texture) {
    return m_textures.has(texture);
}

static GLenum get_filter_value(TextureFilter filter) {
    switch (filter) {
        case TextureFilter::LINEAR:
            return GL_LINEAR;
        case TextureFilter::NEAREST:
            return GL_NEAREST;
        default:
            return 0;
    }
}

static GLenum get_wrap_value(TextureWrap wrap) {
    switch (wrap) {
        case TextureWrap::REPEATED:
            return GL_REPEAT;
        case TextureWrap::CLAMP:
            return GL_CLAMP_TO_EDGE;
        case TextureWrap::MIRRORED_REPEAT:
            return GL_MIRRORED_REPEAT;
        default:
            return 0;
    }
}

SamplerHandle GL_RenderDevice::create_sampler(const SamplerInitDesc &sampler_desc) {
    GL_SamplerStorage storage{};

    glGenSamplers(1, &storage.sampler);
    if (storage.sampler == 0) {
        return SamplerHandle{};
    }

    /* set scale filter parameters */
    glSamplerParameteri(storage.sampler, GL_TEXTURE_MIN_FILTER, get_filter_value(sampler_desc.filter.min));
    glSamplerParameteri(storage.sampler, GL_TEXTURE_MAG_FILTER, get_filter_value(sampler_desc.filter.mag));

    /* set wrap parameters */
    glSamplerParameteri(storage.sampler, GL_TEXTURE_WRAP_S, get_wrap_value(sampler_desc.wrap.u));
    glSamplerParameteri(storage.sampler, GL_TEXTURE_WRAP_T, get_wrap_value(sampler_desc.wrap.v));

    return m_samplers.emplace(storage);
}

void GL_RenderDevice::destroy_sampler(SamplerHandle sampler) {
    auto storage = m_samplers.get_and_destroy(sampler);
    glDeleteSamplers(1, &storage->sampler);
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

    glm::mat4 ortho = glm::ortho(
        0.0f,
        (float)window_viewport.size.x,
        0.0f,
        (float)window_viewport.size.y, -1.0f, 1.0f);

    glUseProgram(m_global_shader->get_program());
    m_global_shader->set_mat4("u_view_proj", ortho);
}

Vec2i GL_RenderDevice::get_screen_size() {
    return m_ctx->get_screen_size();
}

/* drawing */

void GL_RenderDevice::set_pipeline(PipelineHandle pipeline_handle) {
    auto *pipeline = m_pipelines.get(pipeline_handle);
    glBindVertexArray(pipeline->vertex_array);
    glUseProgram(pipeline->shader->get_program());
    m_bound_pipeline = pipeline_handle;
}

void GL_RenderDevice::set_vertex_buffer(BufferHandle vertex_buffer) {
    m_bound_vbo = vertex_buffer;
}

void GL_RenderDevice::set_index_buffer(BufferHandle index_buffer) {
    m_bound_ibo = index_buffer;
}

/**
 * @brief Binds a buffer storage instance with a GL buffer
 *
 * Acts the same as glBindBuffer, except that it asserts
 * if the buffer format stored in the buffer storage is
 * not actually the same as the specified buffer format.
 *
 * @param buffer Buffer storage to bind
 * @param format Buffer destination
 */
FORCE_INLINE void bind_buffer(const GL_BufferStorage &buffer, GLenum format) {
    ASSERT(buffer.format == format, "Buffer handle must reference an vertex buffer");
    glBindBuffer(format, buffer.buffer);
}

/**
 * @brief Binds a pipeline and buffers all at once.
 *
 * This function uses a dumb VAO cache system to
 * minimize driver overhead.
 *
 * All pointers are non-owning.
 *
 * @param pipeline PipelineStorage to bind
 * @param vertex_buffer BufferStorage of a vertex buffer
 * @param index_buffer BufferStorage of an index buffer
 */
static void bind_pipeline_and_buffers(GL_PipelineStorage *pipeline, const GL_BufferStorage *vertex_buffer, const GL_BufferStorage *index_buffer) {
    glBindVertexArray(pipeline->vertex_array);

    /* TODO: maybe reconsider using a hashmap here? */
    if (pipeline->last_used_vbo != vertex_buffer->buffer) {
        bind_buffer(*vertex_buffer, GL_ARRAY_BUFFER);

        /* reconfigure layout for this vbo */
        bind_vertex_layout({
            .attributes = pipeline->attributes,
            .stride = pipeline->stride,
        });

        pipeline->last_used_vbo = vertex_buffer->buffer;
    }

    if (pipeline->last_used_ibo != index_buffer->buffer) {
        bind_buffer(*index_buffer, GL_ELEMENT_ARRAY_BUFFER);
        pipeline->last_used_ibo = index_buffer->buffer;
    }

    if (pipeline->last_used_shader != pipeline->shader->get_program()) {
        GLuint program = pipeline->shader->get_program();
        glUseProgram(program);
        pipeline->last_used_shader = program;
    }
}

static void bind_texture_and_sampler([[maybe_unused]] GL_PipelineStorage *pipeline, uint32_t slot, const GL_TextureStorage *texture, const GL_SamplerStorage *sampler) {
    /* TODO: cache this! (e.g. do not call these functions if they aren't needed) */
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, texture->texture);
    glBindSampler(slot, sampler->sampler);
}

void GL_RenderDevice::set_texture(uint32_t slot, TextureHandle texture, SamplerHandle sampler) {
    /* TODO: do not force 0 as the only slot */
    if (slot == 0) {
        m_bound_texture = texture;
        m_bound_sampler = sampler;
    }
}

void GL_RenderDevice::draw_elements(size_t index_count, size_t index_offset) {
    /* fetch everything */
    auto *pipeline = m_pipelines.get(m_bound_pipeline);
    const auto *vertex_buffer = m_buffers.get(m_bound_vbo);
    const auto *index_buffer = m_buffers.get(m_bound_ibo);
    if (!pipeline || !vertex_buffer || !index_buffer) {
        return;
    }

    const auto *texture = m_bound_texture ? m_textures.get(m_bound_texture) : &m_white_texture;
    const auto *sampler = m_samplers.get(m_bound_sampler);
    if (!texture || !sampler) {
        return;
    }

    bind_pipeline_and_buffers(pipeline, vertex_buffer, index_buffer);

    /* TODO: do not hardcode 0 here. */
    bind_texture_and_sampler(pipeline, 0, texture, sampler);

    const auto count = static_cast<GLsizei>(index_count);
    const auto *indices = reinterpret_cast<void *>(index_offset);
    glDrawElements(GL_TRIANGLES, count, pipeline->index_format, indices);
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
