#pragma once

#include "runtime/renderer/renderer_apis.h"
#include "runtime/server/display_server.h"

#include <memory>

#include "core/assertion.h"
#include "core/logger.h"

#include "sdl3_gl_context.h"

#include <SDL3/SDL.h>

class SDL3_DisplayServer final : public IDisplayServer {
    DisplayWindowHandle init_display(const DisplayWindowProps &props) override {
        SDL_WindowFlags flags = get_common_sdl_flags(props);

        switch (props.api) {
            case RendererAPI::SDL3Renderer:
                break;
            case RendererAPI::OpenGL:
                flags |= SDL_WINDOW_OPENGL;

#if defined(DEBUG)
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

                /* Set OpenGL version */
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
                break;
            default:
                ASSERT(false, "SDL: Invalid renderer API");
                break;
        }

        SDL_Window *window = SDL_CreateWindow(props.caption.data(), props.size.x, props.size.y, flags);
        if (!window) {
            LOG_ERROR("SDL3: Failed to create window. Reason: {0}", SDL_GetError());
            return {};
        }

        return {
            .raw_window_handle = reinterpret_cast<void *>(window),
            .props = props,
        };
    }

    void close_display(DisplayWindowHandle handle) override {
        SDL_DestroyWindow((SDL_Window *)handle.raw_window_handle);
        handle.raw_window_handle = nullptr;
    }

    [[nodiscard]]
    Vec2i get_size(const DisplayWindowHandle &handle) const override {
        auto *window = static_cast<SDL_Window *>(handle.raw_window_handle);

        Vec2i size{};
        SDL_GetWindowSize(window, &size.x, &size.y);
        return size;
    }

    [[nodiscard]]
    std::unique_ptr<IRenderDeviceFactory> get_factory(DisplayWindowHandle handle) override {
        if (handle.props.api == RendererAPI::OpenGL) {
            return std::unique_ptr<IRenderDeviceFactory>(new SDL3_GL_RenderDeviceFactory(
                static_cast<SDL_Window *>(handle.raw_window_handle)));
        }

        /* api not found, no factory to return */
        return std::unique_ptr<IRenderDeviceFactory>{};
    }

private:
    /**
     * @brief Converts a WindowProps object into native SDL window bitfield set, excluding graphical API bits
     * @param props A WindowProps objects to convert
     * @return SDL_WindowFlags object based on windows properties.
     */
    static SDL_WindowFlags get_common_sdl_flags(const DisplayWindowProps &props) {
        SDL_WindowFlags flags{};
        if (props.resizable) {
            flags |= SDL_WINDOW_RESIZABLE;
        }
        return flags;
    }
};
