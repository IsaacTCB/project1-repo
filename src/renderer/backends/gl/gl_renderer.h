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

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    GLRenderer(DisplayWindowHandle& window_data);
    friend Renderer;
};
