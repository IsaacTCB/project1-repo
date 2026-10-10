#include "sdl3_gl_context.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include "core/logger.h"

SDL3_GL_Context::SDL3_GL_Context(SDL_Window *window)
    : m_window(window) {
    m_context = SDL_GL_CreateContext(m_window);
    LOG_TRACE("Started SDL3 + OpenGL context");
}

SDL3_GL_Context::~SDL3_GL_Context() {
    LOG_TRACE("Destroyed SDL3 + OpenGL context");
    SDL_GL_DestroyContext(m_context);
}

void SDL3_GL_Context::bind() {
    if (m_context != SDL_GL_GetCurrentContext()) {
        LOG_TRACE("Swapping contexts");
        SDL_GL_MakeCurrent(m_window, m_context);
    }
}

SDL3_GL_Context::FnLoad SDL3_GL_Context::get_load_proc() {
    return SDL_GL_GetProcAddress;
}

void SDL3_GL_Context::swap_buffers() {
    SDL_GL_SwapWindow(m_window);
}

void SDL3_GL_Context::set_swap_interval(int interval) {
    SDL_GL_SetSwapInterval(interval);
}

Vec2i SDL3_GL_Context::get_screen_size() {
    int w = 0, h = 0;
    SDL_GetWindowSize(m_window, &w, &h);
    return Vec2i{w, h};
}

bool SDL3_GL_Context::is_extension_supported(std::string_view extension_name) {
    return SDL_GL_ExtensionSupported(extension_name.data());
}
