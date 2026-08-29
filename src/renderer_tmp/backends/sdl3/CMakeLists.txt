if(NOT TARGET_PLATFORM STREQUAL "sdl3")
    message(FATAL_ERROR "SDL3 Renderer can only be used on SDL3 platform.")
endif()

add_library(renderer_sdl3 STATIC
    sdl3_renderer.cpp
)

target_link_libraries(renderer_sdl3 PRIVATE SDL3::SDL3 engine::core)
target_compile_definitions(renderer_sdl3 PUBLIC -DRENDERER_SDL3_ENABLED=1)
