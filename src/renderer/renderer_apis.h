#pragma once

enum class RendererAPI {
    SDL3Renderer,
    OpenGL,
};

inline RendererAPI get_default_api() {
    return RendererAPI::OpenGL;
}