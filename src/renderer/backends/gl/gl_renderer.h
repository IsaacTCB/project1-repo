#pragma once

#include "renderer/api/renderer.h"

#include <memory>

class GLRenderer : public Renderer {
public:
    // Destructor MUST be declared in the cpp file,
    // because we are pimpl-ing with an opaque struct.
    ~GLRenderer();

    void clear(float r, float g, float b, float a = 255) override;
    void swap_buffers() override;

    TextureHandle load_texture(ImageData& image) override;
    void destroy_texture(TextureHandle handle) override;

    void draw_texture(TextureHandle handle, int x, int y, int width, int height) override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    GLRenderer(DisplayWindowHandle& window_data);
    friend Renderer;
};
