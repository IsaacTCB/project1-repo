#include "sdl3_renderer.h"

SDL3Renderer::SDL3Renderer(DisplayWindowHandle& window_data) {
    m_window = reinterpret_cast<SDL_Window*>(window_data.raw_window_handle);
    m_renderer = SDL_CreateRenderer(m_window, nullptr);
    SDL_SetRenderVSync(m_renderer, 1);
}

void SDL3Renderer::clear(float r, float g, float b, float a) {
    SDL_SetRenderDrawColor(m_renderer,
        static_cast<uint8_t>(r * 255.0f),
        static_cast<uint8_t>(g * 255.0f),
        static_cast<uint8_t>(b * 255.0f),
        static_cast<uint8_t>(a * 255.0f));
    SDL_RenderClear(m_renderer);
}

void SDL3Renderer::swap_buffers() {
    SDL_RenderPresent(m_renderer);
}
