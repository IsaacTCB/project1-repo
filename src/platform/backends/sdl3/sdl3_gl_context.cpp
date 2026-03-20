#include "renderer/gl/gl_context.h"

#include <SDL3/SDL_video.h>

#include "core/logger.h"

struct GLContext::Impl {
    SDL_Window *window;
    SDL_GLContext context;
};

GLContext::GLContext()
    :load_func(SDL_GL_GetProcAddress),
    m_impl{} {
}

GLContext::~GLContext() {
    close();
}

void GLContext::init(void *raw_window_handle) {
    m_impl.reset(new GLContext::Impl);
    m_impl->window = reinterpret_cast<SDL_Window*>(raw_window_handle);
    m_impl->context = SDL_GL_CreateContext(m_impl->window);

    LOG_TRACE("Started SDL3 + OpenGL context");
}

void GLContext::close() {
    if (m_impl) {
        LOG_TRACE("OpenGL SDL3 + GL context");
        SDL_GL_DestroyContext(m_impl->context);
        m_impl.reset(nullptr);
    }
}

void GLContext::bind() {
    if (m_impl->context != SDL_GL_GetCurrentContext()) {
        LOG_TRACE("Swapping contexts");
        SDL_GL_MakeCurrent(m_impl->window, m_impl->context);
    }
}

void GLContext::swap_buffers() {
    SDL_GL_SwapWindow(m_impl->window);
}

void GLContext::set_swap_interval(int interval) {
    SDL_GL_SetSwapInterval(interval);
}
