add_subdirectory(${PROJECT_SOURCE_DIR}/external/glad glad)
add_subdirectory(${PROJECT_SOURCE_DIR}/external/glm glm)

add_library(renderer_gl STATIC
    gl_render_device.cpp
    gl_shader.cpp
)

target_link_libraries(renderer_gl PUBLIC glad glm engine::core)
target_compile_definitions(renderer_gl PUBLIC -DRENDERER_GL_ENABLED=1)
