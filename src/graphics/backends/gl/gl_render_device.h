#pragma once

#include <memory>
#include <optional>

#include "core/math/vec2i.h"
#include "core/utils/handles.h"
#include "core/viewport.h"
#include "graphics/api/render_device.h"
#include "graphics/api/render_device_factory.h"

#include "gl_context.h"
#include "gl_shader.h"

#include <glad/gl.h>

struct GL_PipelineStorage {
    GLuint vertex_array;
    GL_Shader *shader; /* temporary */
};

struct GL_BufferStorage {
    GLuint buffer;
    GLenum format;
    GLenum usage;
    GLsizeiptr size;
};

class GL_RenderDevice final : public IRenderDevice {
public:
    GL_RenderDevice() = delete;
    GL_RenderDevice(std::unique_ptr<GL_IContext> &&context);
    ~GL_RenderDevice() override;

protected:
    void begin_frame() override;
    void end_frame() override;

    /* texture actions */
    TextureHandle load_texture(const ImageData &data) override;
    void destroy_texture(TextureHandle texture) override;
    bool has_texture(TextureHandle texture) override;

    /* pipeline operations */
    PipelineHandle create_pipeline(const PipelineDesc &pipeline_desc) override;
    void destroy_pipeline(PipelineHandle pipeline) override;

    /* buffer operations */
    BufferHandle create_buffer(const BufferInitDesc &buffer_desc) override;
    void update_buffer(BufferHandle buffer, size_t size, void *data, size_t offset) override;
    void destroy_buffer(BufferHandle buffer) override;

    void clear(const Color &color) override;

    Viewport build_viewport(const Vec2i &size) override;
    void apply_viewport(const Viewport &screen_viewport, const Viewport &window_viewport) override;

    Vec2i get_screen_size() override;

private:
    void print_info();

private:
    std::unique_ptr<GL_IContext> m_ctx;

    std::optional<GL_Shader> m_global_shader;
    HandlePool<GL_PipelineStorage> m_pipelines;
    HandlePool<GL_BufferStorage> m_buffers;
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
