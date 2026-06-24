cmake_minimum_required(VERSION 3.11)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(engine_platform)
include(sdl3_utils)

# Fetches SDL3
find_or_download_sdl3()

set(SDL3_SOURCES
    sdl3_common.cpp
    sdl3_gamepad.cpp
    sdl3_platform.cpp
    sdl3_main.cpp
)

# Create target with ${TARGET_NAME}
create_sdl3_target(${SDL3_SOURCES})
engine_add_platform(${TARGET_NAME})

if(EMSCRIPTEN)
    target_compile_definitions(${TARGET_NAME} PRIVATE USE_THREADS=0)
    set_target_properties(${TARGET_NAME} PROPERTIES SUFFIX ".html")
else()
    target_compile_definitions(${TARGET_NAME} PRIVATE USE_THREADS=1)
endif()

if(MINGW)
    target_link_options(${TARGET_NAME}
        PRIVATE
        -static-libgcc
        -static-libstdc++
        -static
    )
endif()

# Enable SDL callback
target_compile_definitions(${TARGET_NAME} PRIVATE SDL_MAIN_USE_CALLBACKS)
target_link_libraries(${TARGET_NAME}
    PRIVATE
    SDL3::SDL3
)

engine_add_renderer(${TARGET_NAME} sdl3)
engine_add_renderer(${TARGET_NAME} gl sdl3_gl_context.cpp)

if(WIN32)
    engine_copy_dll(
        ${TARGET_NAME}
        SDL3::SDL3)
endif()
