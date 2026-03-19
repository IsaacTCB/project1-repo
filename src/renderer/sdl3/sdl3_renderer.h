#pragma once

#include "../renderer.h"

class SDL3Renderer : public Renderer {
public:
    void clear(float r, float g, float b, float a = 255) override;
    void swap_buffers() override;

private:
    struct Impl;
    Impl *m_impl;

    SDL3Renderer(DisplayWindowHandle& window_data);
    friend Renderer;
};
