# TODO: this whole file needs to be revised and rewritten from scratch.
# It's really messy and poluted and hacky, I hate it.

cmake_minimum_required(VERSION 3.11)

option(ENGINE_SDL_FETCH_CONTROLLERDB "Download gamecontrollerdb.txt automatically" ON)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(engine_platform)
include(sdl3_utils)

# Fetches SDL3
find_or_download_sdl3()

set(SDL3_SOURCES
    sdl3_common.cpp
    sdl3_gamepad.cpp
    sdl3_platform.cpp
    sdl3_registry.cpp
)

add_library(sdl3_platform_runtime
    STATIC
    ${SDL3_SOURCES}
)

engine_add_platform(sdl3_platform_runtime)

# mingw builds needs the entrypoint to be linked
# against the main executable, so that its linker
# recognizes WinMain (we are using SDL callbacks)
add_library(sdl3_platform_main OBJECT sdl3_main.cpp)

# Enable SDL callbacks
target_compile_definitions(sdl3_platform_main PRIVATE SDL_MAIN_USE_CALLBACKS)

engine_add_platform(sdl3_platform_main)
target_link_libraries(sdl3_platform_main PRIVATE SDL3::SDL3)

# engine_add_renderer(sdl3_platform_runtime sdl3)
engine_add_renderer(sdl3_platform_runtime gl
    ${CMAKE_CURRENT_SOURCE_DIR}/sdl3_gl_context.cpp
)

if(ENGINE_ENABLE_IMGUI)
    target_sources(sdl3_platform_runtime PRIVATE
        ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
        sdl3_gl_imgui.cpp)
    target_link_libraries(sdl3_platform_runtime PRIVATE imgui)
endif()

target_link_libraries(sdl3_platform_runtime
    PRIVATE
    SDL3::SDL3
)

if(ENGINE_SDL_FETCH_CONTROLLERDB)
    fetch_gamecontrollerdb()
endif()

function(engine_create_application target_name)
    create_sdl3_target(${target_name} ${ARGN})
    target_link_libraries(${target_name} PRIVATE sdl3_platform_main sdl3_platform_runtime)
    if(TARGET imgui)
        target_link_libraries(${target_name} PRIVATE imgui)
        target_compile_definitions(${target_name} PRIVATE ENABLE_IMGUI=1)
    endif()
    # target_link_libraries(${target_name} PRIVATE engine::core)
    # target_link_libraries(${target_name} PRIVATE engine::runtime)

    # if(EMSCRIPTEN)
    #     target_compile_definitions(${target_name} PRIVATE USE_THREADS=0)
    #     set_target_properties(${target_name} PROPERTIES SUFFIX ".html")
    # else()
    #     target_compile_definitions(${target_name} PRIVATE USE_THREADS=1)
    # endif()

    if(MINGW)
        target_link_options(${target_name}
            PRIVATE
            -static-libgcc
            -static-libstdc++
            -static
        )
    endif()

    if(ENGINE_SDL_FETCH_CONTROLLERDB AND NOT EMSCRIPTEN)
        if(EXISTS "${sdl_controller_db_FILEPATH}")
            add_custom_command(
                TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${sdl_controller_db_FILEPATH}"
                        "$<TARGET_FILE_DIR:${target_name}>/gamecontrollerdb.txt"
                DEPENDS "${sdl_controller_db_FILEPATH}"
                COMMENT "Copying 'gamecontrollerdb.txt'..."
            )
        else()
            message(STATUS "Notice: gamecontrollerdb.txt was not downloaded.")
        endif()
    endif()

    engine_copy_dll(${target_name})
endfunction()

