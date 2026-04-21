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
    set(CMAKE_BUILD_TYPE ${default_build_type} CACHE STRING "Chooce the build type." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # HACK: This command affects all targets, including dependencies.
    # This should get replaced by target_compile_definitions().
    add_compile_definitions("-DENABLE_ASSERTS=1")
endif()

# Used by platform projects to add renderers.
function(target_add_renderer target name)
    add_subdirectory(
        ${CMAKE_SOURCE_DIR}/src/renderer/backends/${name}
        ${CMAKE_CURRENT_BINARY_DIR}/renderer_${name})
    target_include_directories(renderer_${name} PUBLIC ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(${target} PRIVATE renderer_${name})

    message(STATUS "Linked renderer ${name} to ${target}")
endfunction()

# Common stuff included here too.
add_library(engine STATIC
    src/test/testapp.cpp
    src/core/logger.cpp
    src/core/image/decoder.cpp
    src/platform/api/platform.cpp
    src/platform/api/application_handler.cpp
    src/renderer/api/renderer.cpp)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/stb_image)
target_link_libraries(engine PRIVATE stb_image)

target_include_directories(engine PUBLIC src)

# Platform (entrypoint)
if(PLATFORM_SDL3)
    add_subdirectory(
        src/platform/backends/sdl3
        ${CMAKE_CURRENT_BINARY_DIR}/sdl3
    )
endif()
