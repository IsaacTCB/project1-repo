#pragma once

#include <memory>
#include <optional>

#include "core/math/vec2i.h"
#include "core/math/vec3.h"
#include "core/utils/handle_pool.h"
#include "core/viewport.h"
#include "graphics/api/render_device.h"
#include "graphics/api/render_device_factory.h"
#include "graphics/api/render_device_handles.h"
#include "graphics/api/vertex_layout.h"

#include "gl_context.h"
#include "gl_shader.h"
#include "runtime/renderer/render_handles.h"

#include <glad/gl.h>
#include <glm/glm.hpp>

struct GL_PipelineStorage {
    GLuint vertex_array{};

    /* settings */
    GLenum index_format;

    /* cache */
    GLuint last_used_vbo{};
    GLuint last_used_ibo{};
    GLuint last_used_shader{};

    struct Blending {
        bool enabled;
        GLenum src;
        GLenum dst;

    } blending;

    /* layout */
    std::vector<VertexAttribute> attributes;
    size_t stride;

    /* bound objects */
    GL_Shader *shader; /* TODO: replace this with a shader handle */
};

struct GL_BufferStorage {
    GLuint buffer;
    GLenum format;
    GLenum usage;
    GLsizeiptr size;
};

struct GL_TextureStorage {
    GLuint texture;

    /* format stuff */
    GLenum internal_format;
    GLenum data_format;
};

struct GL_SamplerStorage {
    GLuint sampler;
};

struct GL_MatrixStorage {
    glm::mat4 matrix;
};

struct GL_RenderTargetStorage {
    GLuint framebuffer;

    TextureHandle texture;
    /* TODO: depth */

    Vec2i size;
};

class GL_RenderDevice final : public IRenderDevice {
public:
    GL_RenderDevice() = delete;
    GL_RenderDevice(std::unique_ptr<GL_IContext> &&context);
    ~GL_RenderDevice() override;

public:
    const char *name() const override { return "gl"; }

    void begin_frame() override {}
    void end_frame() override {}
    void present() override;

    void begin_pass(const RenderPassApplyDesc &pass_desc) override;
    void end_pass() override;

    RenderTargetHandle create_render_target(const RenderTargetDesc &render_target_desc) override;
    void resize_render_target(RenderTargetHandle handle, Vec2i size) override;
    void destroy_render_target(RenderTargetHandle handle) override;
    TextureHandle get_color_attachment(RenderTargetHandle handle) override;

    /* texture actions */
    TextureHandle load_texture(const ImageData &data) override;
    void destroy_texture(TextureHandle texture) override;
    bool has_texture(TextureHandle texture) override;
    std::optional<RenderTextureInfo> get_texture_info(TextureHandle texture) const override;

    SamplerHandle create_sampler(const SamplerInitDesc &sampler_desc) override;
    void destroy_sampler(SamplerHandle sampler) override;

    /* pipeline operations */
    PipelineHandle create_pipeline(const PipelineDesc &pipeline_desc) override;
    void destroy_pipeline(PipelineHandle pipeline) override;

    /* buffer operations */
    BufferHandle create_buffer(const BufferInitDesc &buffer_desc) override;
    void update_buffer(BufferHandle buffer, size_t size, const void *data, size_t offset) override;
    void destroy_buffer(BufferHandle buffer) override;

    /* matrix operations */
    MatrixHandle create_matrix() override;
    void destroy_matrix(MatrixHandle matrix) override;

    void matrix_load_identity(MatrixHandle matrix) override;
    void matrix_ortho(MatrixHandle matrix, float left, float right, float top, float bottom, float near = -1.0f, float far = 1.0f) override;
    void matrix_translate_rotate_scale(MatrixHandle matrix, const Vec3 &translate, float rotate, const Vec3 &rotate_axis, const Vec3 &scale, const Vec3 &offset) override;

    void clear(const Color &color) override;

    void set_pipeline(PipelineHandle pipeline) override;
    void set_mvp_matrix(const MatrixApplyDesc &matrix_desc) override;
    void set_draw_buffers(const BufferApplyDesc &buffer_desc) override;
    void set_texture(uint32_t slot, TextureHandle texture, SamplerHandle sampler) override;
    void draw_elements(size_t index_count, size_t index_offset) override;

    Viewport build_viewport(const Vec2i &size) override;
    void apply_viewport(const Viewport &screen_viewport, const Viewport &window_viewport) override;

    Vec2i get_screen_size() override;

private:
    void print_info();

private:
    std::unique_ptr<GL_IContext> m_ctx;

    std::optional<GL_Shader> m_global_shader;
    GL_TextureStorage m_white_texture;

    PipelineHandle m_bound_pipeline;
    BufferHandle m_bound_vbo;
    BufferHandle m_bound_ibo;
    TextureHandle m_bound_texture;
    SamplerHandle m_bound_sampler;
    GLenum m_bound_sfactor = 0;
    GLenum m_bound_dfactor = 0;

    bool m_blending_enabled = true;

    struct MatrixState {
        MatrixHandle projection{};
        MatrixHandle view{};
        MatrixHandle model{};
    } m_bound_matrices{};

    HandlePool<GL_TextureStorage, TextureHandle> m_textures;
    HandlePool<GL_SamplerStorage, SamplerHandle> m_samplers;
    HandlePool<GL_PipelineStorage, PipelineHandle> m_pipelines;
    HandlePool<GL_BufferStorage, BufferHandle> m_buffers;
    HandlePool<GL_MatrixStorage, MatrixHandle> m_matrices;
    HandlePool<GL_RenderTargetStorage, RenderTargetHandle> m_targets;
};

class GL_IRenderDeviceFactory : public IRenderDeviceFactory {
protected:
    GL_RenderDevice *create() override {
        auto context = get_context();
        return new GL_RenderDevice(std::move(context));
    }

    /* TODO: maybe replace this with templates, if possible? */
    virtual std::unique_ptr<GL_IContext> get_context() = 0;
};
