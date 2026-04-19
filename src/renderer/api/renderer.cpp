#include "renderer/api/renderer.h"

#include "../backends/sdl3/sdl3_renderer.h"

#include "core/logger.h"

std::shared_ptr<Renderer> Renderer::create(DisplayWindowHandle& window_data) {
    switch (window_data.props.api) {
        case RendererAPI::SDL3Renderer:
            return std::shared_ptr<Renderer>(new SDL3Renderer(window_data));
        case RendererAPI::OpenGL:
            ASSERT(false, "Renderer::create(): OpenGL renderer is unimplemented.");
        default:
            LOG_CRITICAL("Unsupported graphics API");
            return nullptr;
    }
}
