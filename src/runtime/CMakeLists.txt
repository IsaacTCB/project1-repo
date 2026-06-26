include(engine_utils)

set(RUNTIME_SOURCES
    sandbox_app.cpp
    input_service.cpp
    asset_service.cpp
    image/decoder.cpp
    ${CMAKE_SOURCE_DIR}/src/platform/api/application_handler.cpp
)

# TODO: Somehow remove renderer.cpp from here!
# or refactor its whole architecture!
list(APPEND RUNTIME_SOURCES
    ${CMAKE_SOURCE_DIR}/src/renderer/api/renderer.cpp
)

add_library(runtime STATIC ${RUNTIME_SOURCES})
target_link_libraries(runtime PRIVATE core)
engine_add_module(runtime)

# stb_image
add_subdirectory(${CMAKE_SOURCE_DIR}/external/stb_image external/stb_image)
target_link_libraries(runtime PRIVATE stb_image)
