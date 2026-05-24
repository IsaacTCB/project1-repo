cmake_minimum_required(VERSION 3.11)

set(SDL3_VERSION 3.4)
set(SDL3_FETCH_VERSION 3.4.2)

find_package(SDL3 ${SDL3_VERSION} QUIET)
if(NOT SDL3_FOUND)
    message(VERBOSE "SDL3 installation was not found on system. Downloading SDL3 ${SDL3_VERSION}...")

    include(FetchContent)
    FetchContent_Declare("SDL3"
        GIT_REPOSITORY  https://github.com/libsdl-org/SDL.git
        GIT_TAG         release-${SDL3_FETCH_VERSION}
        GIT_SHALLOW     TRUE
        GIT_PROGRESS    TRUE
    )
    FetchContent_MakeAvailable("SDL3")
    set(SDL3_FOUND TRUE)
endif()

set(SDL3_SOURCES
    sdl3_common.cpp
    sdl3_display_server.cpp
    sdl3_io_server.cpp
    sdl3_os_server.cpp
    sdl3_gamepad.cpp
    sdl3_platform.cpp
    sdl3_main.cpp
)

set(BUILD_DEBUG (CMAKE_BUILD_TYPE STREQUAL "Debug"))

if(ANDROID)
    add_library(main SHARED ${SDL3_SOURCES})
    # target_add_renderer(main sdl3)
    add_library(${TARGET_NAME} ALIAS main)
else()
    if(BUILD_DEBUG)
        add_executable(${TARGET_NAME} ${SDL3_SOURCES})
    else()
        # For non-debug builds, disable console on Windows.
        add_executable(${TARGET_NAME} WIN32 ${SDL3_SOURCES})
    endif()
endif()

target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(${TARGET_NAME} PRIVATE SDL_MAIN_USE_CALLBACKS)

if(EMSCRIPTEN)
    target_compile_definitions(${TARGET_NAME} PRIVATE -DUSE_THREADS=0)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
else()
    target_compile_definitions(${TARGET_NAME} PRIVATE -DUSE_THREADS=1)
endif()

target_link_libraries(${TARGET_NAME}
    PRIVATE
    engine
    PUBLIC
    SDL3::SDL3
)

target_add_renderer(${TARGET_NAME} sdl3)
target_add_renderer_src(${TARGET_NAME} gl sdl3_gl_context.cpp)

if(WIN32)
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}"
        -E copy_if_different
        "$<TARGET_FILE:SDL3::SDL3>"
        "$<TARGET_FILE_DIR:${TARGET_NAME}>"
        VERBATIM
    )
endif()
