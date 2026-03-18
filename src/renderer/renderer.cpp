#include "renderer.h"

#include "sdl3/sdl3_renderer.h"

#include "../platform/common/logger.h"

std::shared_ptr<Renderer> Renderer::create(DisplayWindowHandle& window_data) {
    switch (window_data.props.api) {
        case RendererAPI::SDL3Renderer:
            return std::shared_ptr<Renderer>(new SDL3Renderer(window_data));
        case RendererAPI::OpenGL:
            LOG_ERROR("Renderer::create(): OpenGL renderer is unimplemented.");
            return nullptr;
    }
    return nullptr;
}
