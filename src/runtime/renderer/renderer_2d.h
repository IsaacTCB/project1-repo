#pragma once

#include <memory>

#include "core/image/image.h"
#include "core/math/vec2i.h"
#include "core/view_context_2d.h"
#include "core/viewport.h"

#include "graphics/api/render_device.h"
#include "runtime/app/context/frame_context.h"

#include "batch_builder_2d.h"
#include "render_handles.h"

class IRenderDevice;
class IRenderDeviceFactory;

class Renderer2D {
public:
    Renderer2D();
    ~Renderer2D();

    Renderer2D(const Renderer2D &rhs) = delete;
    Renderer2D &operator=(const Renderer2D &rhs) = delete;
    Renderer2D(Renderer2D &&rhs) noexcept = default;
    Renderer2D &operator=(Renderer2D &&rhs) noexcept = default;

public:
    bool attach_device(std::unique_ptr<IRenderDeviceFactory> factory);
    void init();

public:
    /* ================== *
     *  Frame rendering   *
     * ================== */

    void render_frame(const RenderContext &ctx);

    /* ================== *
     *  Texture Actions   *
     * ================== */

    TextureHandle load_texture(ImageData &data);
    void destroy_texture(TextureHandle texture);

    [[nodiscard]]
    const Viewport &get_viewport() const { return m_screen_viewport; };

    void on_screen_resized(const Vec2i &screen_size);

private:
    bool init_device();
    void close_device();

    void render_frame_pass(const RenderContext &ctx);
    void update_view_projection(const ViewContext2D &view);
    void draw_fullscreen_quad(TextureHandle texture);

private:
    struct MatrixMVP {
        MatrixHandle projection{};
        MatrixHandle view{};
    } m_matrices{};

    bool m_enabled_render_targets{};
    RenderTargetHandle m_render_target{};

    std::optional<BatchBuilder2D> m_batch;
    std::unique_ptr<IRenderDevice> m_rd;
    Viewport m_screen_viewport;
};
