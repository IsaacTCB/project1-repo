add_library(renderer_pspgu STATIC
    pspgu_renderer.cpp
    pspgu_texture.cpp
)

target_link_libraries(renderer_pspgu PRIVATE
    pspge
    pspgu
    pspgum
)
target_compile_definitions(renderer_pspgu PUBLIC -DRENDERER_PSPGU_ENABLED=1)
