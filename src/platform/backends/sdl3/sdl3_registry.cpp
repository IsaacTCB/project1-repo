#include "sdl3_registry.h"

#if RENDERER_GL_ENABLED
#include "sdl3_gl_context.h"
#endif

BackendRegistry<SDL_FactorySpecs> get_backend_registry() {
    BackendRegistry<SDL_FactorySpecs> registry{};

#if RENDERER_GL_ENABLED
    registry.add("gl",
        SDL_FactorySpecs{
            .get_flags = []() -> uint32_t {
#if defined(DEBUG)
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

                return SDL_WINDOW_OPENGL;
            },
            .create = [](SDL_Window *window) -> IRenderDeviceFactory * {
                return new SDL3_GL_RenderDeviceFactory(window);
            },
        });
#endif // RENDERER_GL_ENABLED

    return registry;
}
