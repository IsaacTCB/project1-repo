#include "sdl3_renderer.h"

#include <SDL3/SDL.h>

#include "core/assertion.h"
#include "core/handles.h"

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

TextureHandle SDL3Renderer::load_texture(ImageData& image) {
    ASSERT(image.pixels.size() > 0, "Image is not loaded");

    SDL_PixelFormat format;
    switch (image.channels) {
        case 3:
            format = SDL_PIXELFORMAT_RGB24;
            break;
        case 4:
            format = SDL_PIXELFORMAT_RGBA32;
            break;
        default:
            ASSERT(false, "Invalid bpp");
            break;
    }

    int pitch = image.width * image.channels;

    SDL_Surface* surface = SDL_CreateSurfaceFrom(
        image.width, image.height, format,
        image.pixels.data(), pitch);

    SDL_Texture* texture = SDL_CreateTextureFromSurface(
        m_impl->renderer,
        surface);

    SDL_DestroySurface(surface);
    return TextureHandle{};
}

void SDL3Renderer::destroy_texture(TextureHandle texture) {}

