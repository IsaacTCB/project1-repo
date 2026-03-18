add_library(renderer_sdl3 STATIC
    sdl3_renderer.cpp
)
target_compile_definitions(renderer_sdl3 PUBLIC -DRENDERER_SDL3_ENABLED=1)
