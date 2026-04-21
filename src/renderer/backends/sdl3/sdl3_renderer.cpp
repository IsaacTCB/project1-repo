#include "sdl3_renderer.h"

#include <SDL3/SDL.h>
#include <cassert>

#include "core/assertion.h"
#include "core/handles.h"
#include "core/logger.h"

struct SDL3Renderer::Impl {
    SDL_Window* window;
    SDL_Renderer* renderer;

    HandlePool<SDL_Texture*> texture_pool;
};

SDL3Renderer::SDL3Renderer(DisplayWindowHandle& window_data)
    : m_impl(std::make_unique<Impl>()) {
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
    ASSERT(pitch > 0, "Invalid pitch size: pitch <= 0");

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
    auto slot = m_impl->texture_pool.get_and_destroy(handle);
    if (!slot) {
        ASSERT(false, "Attempted to delete a non-existent handle");
        LOG_ERROR("SDL3Renderer::destroy_texture(): Attempted to delete a non-existent handle");
        return;
    }

    SDL_Texture* texture = *slot;
    SDL_DestroyTexture(texture);
}

void SDL3Renderer::command(const RenderCommandDraw& command) {
    SDL_FRect dest{
        .x = static_cast<float>(command.x),
        .y = static_cast<float>(command.y),
        .w = static_cast<float>(command.width),
        .h = static_cast<float>(command.height),
    };

    if (command.texture.has_value()) {
        Handle handle = command.texture.value();
        SDL_Texture** slot = m_impl->texture_pool.get(handle);
        ASSERT(slot, "Attempted to draw a non-existent handle");

        SDL_Texture* texture = *slot;
        SDL_FRect rect{
            .x = 0.0f,
            .y = 0.0f,
            .w = static_cast<float>(texture->w),
            .h = static_cast<float>(texture->h),
        };
        SDL_RenderTexture(m_impl->renderer, texture, &rect, &dest);
    } else {
        Uint8 r = ((command.color & 0xFF000000) >> 24);
        Uint8 g = ((command.color & 0x00FF0000) >> 16);
        Uint8 b = ((command.color & 0x0000FF00) >> 8);
        Uint8 a = ((command.color & 0x000000FF) >> 0);
        SDL_SetRenderDrawColor(m_impl->renderer, r, g, b, a);
        SDL_RenderFillRect(m_impl->renderer, &dest);
    }
}

void SDL3Renderer::render_pass(const RenderQueue& queue) {
    for (const auto& command: queue.get()) {
        std::visit([this](const auto& cmd) {
            this->command(cmd);
        },
            command);
    }
}
