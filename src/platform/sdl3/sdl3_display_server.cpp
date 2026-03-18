#include "../display_server.h"

#include "sdl3_common.h"

#include <cstdlib>

static SDL_WindowFlags get_common_sdl_flags(const DisplayWindowProps &props);

DisplayWindowHandle DisplayServer::init_display(const DisplayWindowProps& props) {
    SDL_WindowFlags flags = get_common_sdl_flags(props);

    switch (props.api) {
    case RendererAPI::SDL3Renderer:
        break;
    case RendererAPI::OpenGL:
        flags |= SDL_WINDOW_OPENGL;
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        break;
    }

    SDL_Window *window = SDL_CreateWindow(props.caption.data(), props.width, props.height, flags);
    if (!window) {
        /* Ideally, we fallback to other APIs */
        exit(0);
    }

    return {
        .raw_window_handle = reinterpret_cast<void *>(window),
        .props = props,
    };
}

void DisplayServer::close_display(DisplayWindowHandle handle) {
    SDL_DestroyWindow((SDL_Window*)handle.raw_window_handle);
    handle.raw_window_handle = nullptr;
}

std::tuple<uint32_t, uint32_t> DisplayServer::get_size(DisplayWindowHandle handle) const {
    auto window = reinterpret_cast<SDL_Window *>(handle.raw_window_handle);

    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    return {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };
}

/**
 * @brief Converts a WindowProps object into native SDL window bitfield set, excluding graphical API bits
 * @param props A WindowProps objects to convert
 * @return SDL_WindowFlags object based on windows properties.
 */
SDL_WindowFlags get_common_sdl_flags(const DisplayWindowProps &props) {
    SDL_WindowFlags flags{};
    if (props.resizable) {
        flags |= SDL_WINDOW_RESIZABLE;
    }
    return flags;
}
