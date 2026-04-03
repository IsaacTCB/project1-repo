#include "sdl3_renderer.h"

#include <SDL3/SDL.h>

#include "core/assertion.h"
#include "core/handles.h"
#include "core/logger.h"

struct SDL3Renderer::Impl {
    SDL_Window* window;
    SDL_Renderer* renderer;

    HandlePool<SDL_Texture*> texture_pool;
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
    ASSERT(surface, "Unable to create SDL surface");

    SDL_Texture* texture = SDL_CreateTextureFromSurface(
        m_impl->renderer,
        surface);
    ASSERT(texture, "Unable to create SDL texture");

    SDL_DestroySurface(surface);
    return m_impl->texture_pool.emplace(texture);
}

void SDL3Renderer::destroy_texture(TextureHandle handle) {
    SDL_Texture** slot = m_impl->texture_pool.get(handle);
    if (!slot) {
        ASSERT(false, "Attempted to delete a non-existent handle");
        LOG_ERROR("SDL3Renderer::destroy_texture(): Attempted to delete a non-existent handle");
        return;
    }

    SDL_Texture* texture = *slot;
    SDL_DestroyTexture(texture);

    m_impl->texture_pool.remove(handle);
}

void SDL3Renderer::draw_texture(TextureHandle handle, int x, int y) {
    SDL_Texture** slot = m_impl->texture_pool.get(handle);
    ASSERT(slot, "Attempted to draw a non-existent handle");

    SDL_Texture* texture = *slot;
    SDL_FRect rect{
        0.0f,
        0.0f,
        static_cast<float>(texture->w),
        static_cast<float>(texture->h),
    };
    SDL_FRect dest{
        static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(texture->w),
        static_cast<float>(texture->h),
    };
    SDL_RenderTexture(m_impl->renderer, texture, &rect, &dest);
}
