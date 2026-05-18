#include "renderer.h"

#include "../backends/gl/gl_renderer.h"
#include "../backends/sdl3/sdl3_renderer.h"

#include "core/logger.h"
#include "renderer/api/render_view.h"

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

RenderView Renderer::get_render_view(ICamera &target_camera, const Viewport &viewport) {
    return RenderView{
        .camera = &target_camera,
        .viewport = {
            .x = 800.0f * viewport.x,
            .y = 600.0f * viewport.y,
            .width = 800.0f * viewport.width,
            .height = 600.0f * viewport.height,
        },
    };
}
