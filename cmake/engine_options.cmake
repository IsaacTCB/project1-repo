if(OPTIONS_LOADED_)
    message(FATAL_ERROR "Options have already been loaded!")
endif()

# ==================
#   Target options
# ==================
set(TARGET_PLATFORM "sdl3" CACHE STRING "Platform to configure for")
set(TARGET_NAME "game" CACHE STRING "The application's desired target name")

# ==================
#   Engine options
# ==================
option(ENGINE_BUILD_TESTS "Build test suite" ${PROJECT_IS_TOP_LEVEL})
option(ENGINE_BUILD_EXAMPLES "Build built-in engine examples" ${PROJECT_IS_TOP_LEVEL})
if(CMAKE_BUILD_TYPE MATCHES "^(Release|MinSizeRel)$")
    set(ENGINE_ENABLE_IMGUI_DEFAULT OFF)
    set(ENGINE_ENABLE_PROFILER_DEFAULT OFF)
else()
    set(ENGINE_ENABLE_IMGUI_DEFAULT ON)
    set(ENGINE_ENABLE_PROFILER_DEFAULT ON)
endif()
option(ENGINE_ENABLE_IMGUI "Enable ImGui interface" ${ENGINE_ENABLE_IMGUI_DEFAULT})
option(ENGINE_ENABLE_PROFILER "Enable performance profiler" ${ENGINE_ENABLE_PROFILER_DEFAULT})

set(OPTIONS_LOADED_ TRUE)
