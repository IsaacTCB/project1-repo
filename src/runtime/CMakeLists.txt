include(engine_utils)

set(RUNTIME_SOURCES
    asset_service.cpp
    input_service.cpp
    image/decoder.cpp
    renderer/renderer_2d.cpp
    renderer/batch_builder_2d.cpp
    system/view/view_system_2d.cpp
    debug/sinks/file_log_sink.cpp
    runtime.cpp
)

add_library(runtime STATIC ${RUNTIME_SOURCES})
target_link_libraries(runtime PRIVATE core)
engine_add_module(runtime)

if(ENGINE_ENABLE_IMGUI)
    target_sources(runtime PRIVATE
        debug/imgui_backend.cpp
        debug/imgui_style.cpp
    )
    target_link_libraries(runtime PUBLIC imgui)
endif()


# stb_image
add_subdirectory(${PROJECT_SOURCE_DIR}/external/stb_image external/stb_image)
target_link_libraries(runtime PRIVATE stb_image)
