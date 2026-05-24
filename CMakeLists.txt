cmake_minimum_required(VERSION 3.16)

project(Engine VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 11)

set(TARGET_PLATFORM "sdl3" CACHE STRING "Platform to configure for")
set(TARGET_NAME "game" CACHE STRING "Hello world")

if(TARGET_PLATFORM STREQUAL "sdl3")
    set(PLATFORM_SDL3 true)
else()
    message(FATAL_ERROR "Unknown platform \'${TARGET_PLATFORM}\'.")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(default_build_type "Debug")
    message(STATUS "Set the build type to `${default_build_type}` as none was specified.")
    set(CMAKE_BUILD_TYPE ${default_build_type} CACHE STRING "Choose the build type." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# These should be included in ALL engine modules.
# TODO: make a proper macro for adding these!
set(ENGINE_COMPILE_DEFINITIONS)
set(ENGINE_COMPILE_OPTIONS)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND ENGINE_COMPILE_DEFINITIONS "ENABLE_ASSERTS=1")
    if(MSVC)
        list(APPEND ENGINE_COMPILE_OPTIONS /W4)
    else()
        list(APPEND ENGINE_COMPILE_OPTIONS -Wall -Wextra -Wpedantic -Wconversion -Wno-sign-conversion)
    endif()
endif()

# Used by platform projects to add renderers.
function(target_add_renderer target name)
    add_subdirectory(
        ${CMAKE_SOURCE_DIR}/src/renderer/backends/${name}
        ${CMAKE_CURRENT_BINARY_DIR}/renderer_${name})

    set(renderer_target renderer_${name})
    target_include_directories(${renderer_target} PUBLIC ${CMAKE_SOURCE_DIR}/src)
    target_compile_options(${renderer_target} PRIVATE ${ENGINE_COMPILE_OPTIONS})
    target_compile_definitions(${renderer_target} PRIVATE ${ENGINE_COMPILE_DEFINITIONS})

    target_link_libraries(${target} PRIVATE ${renderer_target})
    message(STATUS "Linked renderer ${name} to ${target}")
endfunction()

function(target_add_renderer_src target name sources)
    target_add_renderer(${target} ${name})
    target_sources(${target} PRIVATE ${sources}) # Hacky, I guess.
endfunction()

# Common stuff
add_library(engine STATIC
    src/runtime/testapp.cpp
    src/runtime/input_service.cpp
    src/runtime/image/decoder.cpp
    src/core/logger.cpp
    src/platform/api/platform.cpp
    src/platform/api/application_handler.cpp
    src/renderer/api/renderer.cpp)

target_include_directories(engine PUBLIC src)
target_compile_options(engine PUBLIC ${ENGINE_COMPILE_OPTIONS})
target_compile_definitions(engine PUBLIC ${ENGINE_COMPILE_DEFINITIONS})

add_subdirectory(${CMAKE_SOURCE_DIR}/external/stb_image)
target_link_libraries(engine PRIVATE stb_image)

# Platform (entrypoint)
if(PLATFORM_SDL3)
    add_subdirectory(
        src/platform/backends/sdl3
        ${CMAKE_CURRENT_BINARY_DIR}/sdl3
    )
endif()
