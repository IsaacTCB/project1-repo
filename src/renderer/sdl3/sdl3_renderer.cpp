#include "sdl3_renderer.h"

#include <SDL3/SDL.h>

struct SDL3Renderer::Impl {
    SDL_Window* window;
    SDL_Renderer* renderer;
};

SDL3Renderer::SDL3Renderer(DisplayWindowHandle& window_data) 
    :m_impl(std::make_unique<Impl>()) {
    m_impl->window = reinterpret_cast<SDL_Window*>(window_data.raw_window_handle);
    m_impl->renderer = SDL_CreateRenderer(m_impl->window, nullptr);
    SDL_SetRenderVSync(m_impl->renderer, 1);
}

SDL3Renderer::~SDL3Renderer() {
    SDL_DestroyRenderer(m_impl->renderer);
}

void SDL3Renderer::clear(float r, float g, float b, float a) {
    SDL_SetRenderDrawColor(m_impl->renderer,
        static_cast<uint8_t>(r * 255.0f),
        static_cast<uint8_t>(g * 255.0f),
        static_cast<uint8_t>(b * 255.0f),
        static_cast<uint8_t>(a * 255.0f));
    SDL_RenderClear(m_impl->renderer);
}

void SDL3Renderer::swap_buffers() {
    SDL_RenderPresent(m_impl->renderer);
}
