include(engine_utils)

set(RUNTIME_VERSION_CPP ${CMAKE_CURRENT_BINARY_DIR}/version.cpp)

# Runs every build to ensure git hash/dirty state is updated.
# The file is only rewritten when its content actually changes.
string(TOLOWER ${CMAKE_BUILD_TYPE} RUNTIME_FORMATTED_BUILD_TYPE)
add_custom_target(runtime_version
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${PROJECT_SOURCE_DIR}
        -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/version.cpp.in
        -DOUTPUT=${RUNTIME_VERSION_CPP}
        -DVERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        -DVERSION_MINOR=${PROJECT_VERSION_MINOR}
        -DVERSION_PATCH=${PROJECT_VERSION_PATCH}
        -DBUILD_CONFIG=${RUNTIME_FORMATTED_BUILD_TYPE}
        -P ${PROJECT_SOURCE_DIR}/cmake/engine_version.cmake
    BYPRODUCTS ${RUNTIME_VERSION_CPP}
    COMMENT "Checking git revision"
    VERBATIM
)

set(RUNTIME_SOURCES
    asset_service.cpp
    input_service.cpp
    image/decoder.cpp
    renderer/renderer_2d.cpp
    renderer/batch_builder_2d.cpp
    system/view/view_system_2d.cpp
    debug/sinks/file_log_sink.cpp
    runtime.cpp
    ${RUNTIME_VERSION_CPP}
)

add_library(runtime STATIC ${RUNTIME_SOURCES})
add_dependencies(runtime runtime_version)
target_link_libraries(runtime PRIVATE core)
engine_add_module(runtime)

if(ENGINE_ENABLE_IMGUI)
    target_sources(runtime PRIVATE
        debug/imgui_backend.cpp
        debug/imgui_editor.cpp
        debug/imgui_style.cpp
        debug/sinks/imgui_log_sink.cpp
    )
    target_link_libraries(runtime PUBLIC imgui)
endif()


# stb_image
add_subdirectory(${PROJECT_SOURCE_DIR}/external/stb_image external/stb_image)
target_link_libraries(runtime PRIVATE stb_image)
