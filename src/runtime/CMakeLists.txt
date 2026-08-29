include(engine_utils)

set(RUNTIME_SOURCES
    app/application_handler.cpp
    asset_service.cpp
    input_service.cpp
    image/decoder.cpp
    renderer/renderer.cpp
    runtime.cpp
)

add_library(runtime STATIC ${RUNTIME_SOURCES})
target_link_libraries(runtime PRIVATE core)
engine_add_module(runtime)

# stb_image
add_subdirectory(${PROJECT_SOURCE_DIR}/external/stb_image external/stb_image)
target_link_libraries(runtime PRIVATE stb_image)
