cmake_minimum_required(VERSION 3.16)

project(Engine LANGUAGES C CXX)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 11)

set(PLATFORM "sdl3" CACHE STRING "Platform to configure for")
set(EXECUTABLE_NAME "game" CACHE STRING "Hello world")

if(PLATFORM STREQUAL "sdl3")
    set(PLATFORM_SDL3 true)
endif()

function(target_add_renderer target name)
    add_subdirectory(
        ${CMAKE_SOURCE_DIR}/src/renderer/${name}
        ${CMAKE_CURRENT_BINARY_DIR}/renderer_${name})
    target_link_libraries(${target} PRIVATE renderer_${name})
    message(STATUS "Linked renderer ${name} to ${target}")
endfunction()

# Common stuff
add_library(engine STATIC
    src/test/test.cpp
    src/core/logger.cpp
    src/renderer/renderer.cpp)

# Platform (entrypoint)
if(PLATFORM_SDL3)
    add_subdirectory(src/platform/sdl3 ${CMAKE_CURRENT_BINARY_DIR}/sdl3)
endif()

