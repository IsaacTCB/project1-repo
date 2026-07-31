#include "renderer.h"

#include "../backends/gl/gl_renderer.h"
#include "../backends/sdl3/sdl3_renderer.h"

#include "core/logger.h"

std::unique_ptr<Renderer> Renderer::create(DisplayWindowHandle &window_data) {
    switch (window_data.props.api) {
        case RendererAPI::SDL3Renderer:
            return std::unique_ptr<Renderer>(new SDL3Renderer(window_data));
        case RendererAPI::OpenGL:
            return std::unique_ptr<Renderer>(new GLRenderer(window_data));
        default:
            LOG_CRITICAL("Unsupported graphics API");
            return std::unique_ptr<Renderer>(nullptr);
    }
}

Viewport Renderer::get_absolute_viewport(const Viewport &viewport) {
    return {
        .position = m_screen_viewport.size * viewport.position,
        .size = m_screen_viewport.size * viewport.size,
    };
}
