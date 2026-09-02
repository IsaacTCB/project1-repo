#pragma once

#include <memory>

#include "core/image/image.h"
#include "core/math/vec2i.h"
#include "core/view_context_2d.h"
#include "core/viewport.h"

#include "graphics/api/render_device.h"
#include "runtime/app/context/frame_context.h"

#include "batch_builder.h"
#include "render_handles.h"

class IRenderDevice;
class IRenderDeviceFactory;

class Renderer {
public:
    Renderer();
    ~Renderer();

public:
    bool build_device(std::unique_ptr<IRenderDeviceFactory> factory);

public:
    /* ================== *
     *  Frame rendering   *
     * ================== */

    void render_pass(const RenderContext &ctx);

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

    void update_view_projection(const ViewContext2D &view);

private:
    struct MatrixMVP {
        MatrixHandle projection{};
        MatrixHandle view{};
    } m_matrices{};

    std::optional<BatchBuilder> m_batch;
    std::unique_ptr<IRenderDevice> m_rd;
    Viewport m_screen_viewport;
};
