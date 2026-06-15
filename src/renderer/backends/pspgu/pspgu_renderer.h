#pragma once

#include "renderer/api/render_queue.h"
#include "renderer/api/renderer.h"

#include "core/viewport.h"
#include <memory>

class PSPGuRenderer : public Renderer {
public:
    // Destructor MUST be declared in the cpp file,
    // because we are pimpl-ing with an opaque struct.
    ~PSPGuRenderer() override;

    void clear(Color color) override;
    void swap_buffers() override;
    void on_screen_resized(const Vec2i &screen_size) override;

    TextureHandle load_texture(ImageData &image) override;
    void destroy_texture(TextureHandle handle) override;

    void render_pass(const ViewContext2D &view, const RenderQueue &queue) override;

private:
    void apply_viewport(const Viewport &viewport);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    PSPGuRenderer(DisplayWindowHandle &window_data);
    friend Renderer;
};
