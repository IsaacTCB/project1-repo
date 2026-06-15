#pragma once

#include <cstdint>

enum class RendererAPI : uint8_t {
    SDL3Renderer,
    OpenGL,
    PSPGu,
};

/**
 * @brief Gets the default renderer API.
 *
 * @return An RendererAPI for use.
 */
inline RendererAPI get_default_api() {
    return RendererAPI::SDL3Renderer;
}
