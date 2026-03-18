#pragma once

#include <SDL3/SDL.h>

#include "../renderer.h"

class SDL3Renderer : public Renderer {
public:
    void clear(float r, float g, float b, float a = 255) override;
    void swap_buffers() override;

private:
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;

    SDL3Renderer(DisplayWindowHandle& window_data);
    friend Renderer;
};