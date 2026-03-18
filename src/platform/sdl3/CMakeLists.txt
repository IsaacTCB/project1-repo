cmake_minimum_required(VERSION 3.11)

find_package(SDL3 REQUIRED)

set(SDL3_SOURCES
    sdl3_common.cpp
    sdl3_display_server.cpp
    sdl3_io_server.cpp
    sdl3_os_server.cpp
    sdl3_main.cpp
)

if(ANDROID)
    add_library(main SHARED ${SDL3_SOURCES})
    target_add_renderer(main sdl3)
    add_library(${PROJECT_NAME} ALIAS main)
else()
    add_executable(${PROJECT_NAME} ${SDL3_SOURCES})
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    engine
    PUBLIC
    SDL3::SDL3
)

target_add_renderer(${PROJECT_NAME} sdl3)
