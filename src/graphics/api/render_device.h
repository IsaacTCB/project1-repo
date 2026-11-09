#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include "core/image/image.h"
#include "core/math/color.h"
#include "core/math/vec2.h"
#include "core/math/vec2i.h"
#include "core/math/vec3.h"
#include "core/viewport.h"
#include "runtime/renderer/render_handles.h"

#include "render_device_handles.h"
#include "vertex_layout.h"

enum class IndexFormat : uint8_t {
    UINT32,
    UINT16,
    UINT8,
};

enum class BufferType : uint8_t {
    VERTEX,
    INDEX,
};

enum class BufferUsage : uint8_t {
    STATIC,
    DYNAMIC,
    STREAMED,
};

enum class BlendFactor : uint8_t {
    ZERO,
    ONE,
    SRC_COLOR,
    ONE_MINUS_SRC_COLOR,
    DST_COLOR,
    ONE_MINUS_DST_COLOR,
    SRC_ALPHA,
    ONE_MINUS_SRC_ALPHA,
    DST_ALPHA,
    ONE_MINUS_DST_ALPHA,
    CONSTANT_COLOR,
    ONE_MINUS_CONSTANT_COLOR,
    CONSTANT_ALPHA,
    ONE_MINUS_CONSTANT_ALPHA,
    SRC_ALPHA_SATURATE,
};

struct PipelineDesc {
    struct Blending {
        bool enabled = true;
        BlendFactor src = BlendFactor::SRC_ALPHA;
        BlendFactor dst = BlendFactor::ONE_MINUS_SRC_ALPHA;
    } blending{};

    /* TODO: implement these attributes */
    /*
     * struct DepthProps {
     *     bool test = false;
     *     bool write = false;
     * } depth{};
     *
     * bool cull_backface = false;
     */

    IndexFormat index_format = IndexFormat::UINT32;
    VertexLayout vertex_layout;
};

enum class TextureFilter : uint8_t {
    LINEAR,
    NEAREST,
};

enum class TextureWrap : uint8_t {
    REPEATED,
    CLAMP,
    MIRRORED_REPEAT,
};

struct RenderTextureInfo {
    /* TODO: add width, height, format, filtering, etc... */
    void *native_ptr;
};

struct SamplerInitDesc {
    struct FilterProps {
        TextureFilter mag = TextureFilter::LINEAR;
        TextureFilter min = TextureFilter::LINEAR;
    } filter{};

    struct WrapProps {
        TextureWrap u = TextureWrap::REPEATED;
        TextureWrap v = TextureWrap::REPEATED;
    } wrap{};
};

struct BufferInitDesc {
    BufferType type;
    BufferUsage usage;
    size_t size;
    const void *ptr = nullptr;
};

struct BufferApplyDesc {
    BufferHandle vertex;
    BufferHandle index = {};
};

struct MatrixApplyDesc {
    MatrixHandle projection{};
    MatrixHandle view{};
    MatrixHandle model{};
};

struct RenderPassApplyDesc {
    RenderTargetHandle target{};
    Viewport viewport{};
    Color clear_color{};
};

struct RenderTargetDesc {
    Vec2i size{};
};

class IRenderDevice {
public:
    virtual ~IRenderDevice() = default;

protected:
    IRenderDevice() = default;
    friend class Renderer;

public:
    /**
     * @brief Gets render device as a non-owning pointer.
     *
     * @return RenderDevice reference or nullptr if it doesn't exist
     */
    [[nodiscard]]
    virtual const char *name() const = 0;

    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;
    virtual void present() = 0;

    virtual void begin_pass(const RenderPassApplyDesc &pass_desc) = 0;
    virtual void end_pass() = 0;

    virtual RenderTargetHandle create_render_target(const RenderTargetDesc &render_target_desc) = 0;
    virtual void resize_render_target(RenderTargetHandle handle, Vec2i size) = 0;
    virtual void destroy_render_target(RenderTargetHandle handle) = 0;
    virtual TextureHandle get_color_attachment(RenderTargetHandle handle) = 0;

    /* texture actions */
    virtual TextureHandle load_texture(const ImageData &data) = 0;
    virtual void destroy_texture(TextureHandle texture) = 0;
    virtual bool has_texture(TextureHandle texture) = 0;
    [[nodiscard]] virtual std::optional<RenderTextureInfo> get_texture_info(TextureHandle texture) const = 0;

    virtual SamplerHandle create_sampler(const SamplerInitDesc &sampler_desc) = 0;
    virtual void destroy_sampler(SamplerHandle sampler) = 0;

    /* pipeline operations */
    virtual PipelineHandle create_pipeline(const PipelineDesc &pipeline_desc) = 0;
    virtual void destroy_pipeline(PipelineHandle pipeline) = 0;

    /* buffer operations */
    virtual BufferHandle create_buffer(const BufferInitDesc &buffer_desc) = 0;
    virtual void update_buffer(BufferHandle buffer, size_t size, const void *data, size_t offset = 0) = 0;
    virtual void destroy_buffer(BufferHandle buffer) = 0;

    /* matrix operations */
    virtual MatrixHandle create_matrix() = 0;
    virtual void destroy_matrix(MatrixHandle matrix) = 0;

    virtual void matrix_load_identity(MatrixHandle matrix) = 0;
    virtual void matrix_ortho(MatrixHandle matrix, float left, float right, float top, float bottom, float near = -1.0f, float far = 1.0f) = 0;
    /* TODO: replace this with a matrix operation queue/list */
    virtual void matrix_translate_rotate_scale(MatrixHandle matrix, const Vec3 &translate, float rotate, const Vec3 &rotate_axis, const Vec3 &scale, const Vec3 &offset) = 0;

    /* draw */
    virtual void clear(const Color &color) = 0;

    virtual void set_pipeline(PipelineHandle pipeline) = 0;
    virtual void set_mvp_matrix(const MatrixApplyDesc &matrix_desc) = 0;
    virtual void set_draw_buffers(const BufferApplyDesc &buffer_desc) = 0;
    virtual void set_texture(uint32_t slot, TextureHandle texture, SamplerHandle sampler) = 0;
    virtual void draw_elements(size_t index_count, size_t index_offset = 0) = 0;

    virtual Viewport build_viewport(const Vec2i &size) = 0;
    virtual void apply_viewport(const Viewport &screen_viewport, const Viewport &window_viewport) = 0;

    /* currently only used for fetching the screen size when the renderer device gets attached. */
    virtual Vec2i get_screen_size() = 0;
};
