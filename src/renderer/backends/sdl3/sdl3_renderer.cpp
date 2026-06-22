#include "sdl3_renderer.h"

#include <cassert>

#include <SDL3/SDL.h>

#include "core/assertion.h"
#include "core/handles.h"
#include "core/logger.h"
#include "core/math/vec2i.h"
#include "renderer/api/render_handles.h"

struct SDL3Renderer::Impl {
    SDL_Window *window;
    SDL_Renderer *renderer;

    HandlePool<SDL_Texture *> texture_pool;
};

SDL3Renderer::SDL3Renderer(DisplayWindowHandle &window_data)
    : m_impl(std::make_unique<Impl>()) {
    m_impl->window = reinterpret_cast<SDL_Window *>(window_data.raw_window_handle);
    m_impl->renderer = SDL_CreateRenderer(m_impl->window, nullptr);
    SDL_SetRenderDrawBlendMode(m_impl->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderVSync(m_impl->renderer, 1);
}

SDL3Renderer::~SDL3Renderer() {
    SDL_DestroyRenderer(m_impl->renderer);
}

void SDL3Renderer::clear(Color color) {
    SDL_SetRenderDrawColor(m_impl->renderer, color.r, color.g, color.b, color.a);
    SDL_RenderClear(m_impl->renderer);
}

void SDL3Renderer::swap_buffers() {
    SDL_RenderPresent(m_impl->renderer);
}

void SDL3Renderer::on_screen_resized(const Vec2i &screen_size) {
    SDL_SetRenderLogicalPresentation(m_impl->renderer, screen_size.x, screen_size.y, SDL_LOGICAL_PRESENTATION_STRETCH);
}

TextureHandle SDL3Renderer::load_texture(ImageData &image) {
    if (!image) {
        return TextureHandle{};
    }

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
            return TextureHandle{};
    }

    auto pitch = static_cast<int>(image.width * image.channels);
    ASSERT(pitch > 0, "Invalid pitch size: pitch <= 0");

    SDL_Surface *surface = SDL_CreateSurfaceFrom(
        (int)image.width, (int)image.height, format,
        image.pixels.data(), pitch);
    if (!surface) {
        LOG_ERROR("SDL3Renderer::load_texture(): Failed to create image surface!\nreason: {}", SDL_GetError());
        return TextureHandle{};
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(
        m_impl->renderer,
        surface);
    if (!texture) {
        SDL_DestroySurface(surface);
        LOG_ERROR("SDL3Renderer::load_texture(): Failed to create texture from surface!\nreason: {}", SDL_GetError());
        return TextureHandle{};
    }

    SDL_DestroySurface(surface);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    return m_impl->texture_pool.emplace(texture);
}

void SDL3Renderer::destroy_texture(TextureHandle handle) {
    auto slot = m_impl->texture_pool.get_and_destroy(handle);
    if (!slot) {
        ASSERT(false, "Attempted to delete a non-existent handle");
        LOG_ERROR("SDL3Renderer::destroy_texture(): Attempted to delete a non-existent texture handle '{}'", handle);
        return;
    }

    SDL_Texture *texture = *slot;
    SDL_DestroyTexture(texture);
}

void SDL3Renderer::command(const RenderCommandDraw &command) {
    SDL_FRect dest{
        .x = static_cast<float>(command.x),
        .y = static_cast<float>(command.y),
        .w = static_cast<float>(command.width),
        .h = static_cast<float>(command.height),
    };

    auto r = command.color.r;
    auto g = command.color.g;
    auto b = command.color.b;
    auto a = command.color.a;

    Handle handle = command.texture;
    if (auto **slot = m_impl->texture_pool.get(handle)) {
        SDL_Texture *texture = *slot;
        SDL_FRect rect{
            .x = 0.0f,
            .y = 0.0f,
            .w = static_cast<float>(texture->w),
            .h = static_cast<float>(texture->h),
        };
        SDL_SetTextureColorMod(texture, r, g, b);
        SDL_SetTextureAlphaMod(texture, a);
        SDL_RenderTexture(m_impl->renderer, texture, &rect, &dest);
    } else {
        SDL_SetRenderDrawColor(m_impl->renderer, r, g, b, a);
        SDL_RenderFillRect(m_impl->renderer, &dest);
    }
}

void SDL3Renderer::render_pass(const RenderQueue &queue) {
    for (const auto &command : queue.get()) {
        std::visit([this](const auto &cmd) {
            this->command(cmd);
        },
            command);
    }
}
