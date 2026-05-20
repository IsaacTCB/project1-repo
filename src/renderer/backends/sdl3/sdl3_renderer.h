#pragma once

#include "renderer/api/renderer.h"

#include <memory>

class SDL3Renderer : public Renderer {
public:
    // Destructor MUST be declared in the cpp file,
    // because we are pimpl-ing with an opaque struct.
    ~SDL3Renderer() override;

    void clear(float r, float g, float b, float a = 255) override;
    void swap_buffers() override;
    void on_screen_resized(uint32_t width, uint32_t height) override;

    TextureHandle load_texture(ImageData &image) override;
    void destroy_texture(TextureHandle handle) override;

    void render_pass(const ViewContext2D &view, const RenderQueue &queue) override;

private:
    template <typename T>
    void command(T &) = delete;
    void command(const RenderCommandDraw &command);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    SDL3Renderer(DisplayWindowHandle &window_data);
    friend Renderer;
};
