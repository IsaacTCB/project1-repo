set(RUNTIME_SOURCES
    sandbox_app.cpp
    input_service.cpp
    asset_service.cpp
    image/decoder.cpp
    ${CMAKE_SOURCE_DIR}/src/platform/api/application_handler.cpp

    # TODO: Somehow remove renderer.cpp from here!
    # or refactor its whole architecture!
    ${CMAKE_SOURCE_DIR}/src/renderer/api/renderer.cpp
)

add_library(runtime STATIC ${RUNTIME_SOURCES})
target_link_libraries(runtime PRIVATE core)

target_include_directories(runtime PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_compile_options(runtime PRIVATE ${ENGINE_COMPILE_OPTIONS})
target_compile_definitions(runtime PRIVATE ${ENGINE_COMPILE_DEFINITIONS})

# stb_image
add_subdirectory(${CMAKE_SOURCE_DIR}/external/stb_image external/stb_image)
target_link_libraries(runtime PRIVATE stb_image)

