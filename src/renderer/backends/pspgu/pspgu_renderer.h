#pragma once

#include <memory>

#include "renderer/api/render_queue.h"
#include "renderer/api/renderer.h"

class PSPGuTexture;

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
    template <typename T>
    void command(T &) = delete;
    void command(const RenderCommandDraw &command);

private:
    void bind_texture(const PSPGuTexture &texture);
    void unbind_texture();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    PSPGuRenderer(DisplayWindowHandle &window_data);
    friend Renderer;
};
