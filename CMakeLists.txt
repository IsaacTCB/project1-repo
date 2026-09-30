cmake_minimum_required(VERSION 3.16)

project(Engine VERSION 0.1.0 LANGUAGES C CXX)

# ==================
#    Init project
# ==================

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(engine_options)
include(engine_defaults)
include(engine_utils)

# ==================
#   Engine modules
# ==================

add_subdirectory(src/core core)
add_library(engine::core ALIAS core)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui)
if(ENGINE_ENABLE_IMGUI)
    add_library(imgui STATIC
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/backends/imgui_impl_null.cpp
    )
    target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
endif()

add_subdirectory(src/runtime runtime)
add_library(engine::runtime ALIAS runtime)

# ==================
#   Add platforms
# ==================

# Detect the correct platform
engine_detect_platform()

if(PLATFORM_SDL3)
    add_subdirectory(
        src/platform/backends/sdl3
        ${CMAKE_CURRENT_BINARY_DIR}/sdl3
    )
endif()

# ==================
#   Build Examples
# ==================

if(ENGINE_BUILD_EXAMPLES)
    add_subdirectory(apps)
endif()

# ==================
#       Tests
# ==================

if(ENGINE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
