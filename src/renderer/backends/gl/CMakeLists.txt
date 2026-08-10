add_subdirectory(${PROJECT_SOURCE_DIR}/external/glad glad)
add_subdirectory(${PROJECT_SOURCE_DIR}/external/glm glm)

add_library(renderer_gl STATIC
    gl_renderer.cpp
    gl_shader.cpp
    gl_batchbuilder.cpp
)

target_link_libraries(renderer_gl
    PRIVATE
    SDL3::SDL3
)

target_link_libraries(renderer_gl PRIVATE glad glm engine::core)
target_compile_definitions(renderer_gl PUBLIC -DRENDERER_GL_ENABLED=1)

