#pragma once

#include "../renderer.h"

#include <memory>

class SDL3Renderer : public Renderer {
public:
    // Destructor MUST be declared in the cpp file,
    // because we are pimpl-ing with an opaque struct.
    ~SDL3Renderer();

    void clear(float r, float g, float b, float a = 255) override;
    void swap_buffers() override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    SDL3Renderer(DisplayWindowHandle& window_data);
    friend Renderer;
};
