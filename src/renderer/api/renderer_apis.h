#pragma once

enum class RendererAPI {
    SDL3Renderer,
    OpenGL,
};

/**
 * @brief Gets the default renderer API.
 *
 * @return An RendererAPI for use.
 */
inline RendererAPI get_default_api() {
    return RendererAPI::SDL3Renderer;
}
