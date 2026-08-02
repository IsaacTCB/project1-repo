if(DEFAULTS_LOADED_)
    message(FATAL_ERROR "Defaults have already been loaded!")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 11)

# Engine-wide options
set(ENGINE_COMPILE_DEFINITIONS)
set(ENGINE_COMPILE_OPTIONS)

if(NOT CMAKE_BUILD_TYPE)
    set(default_build_type "Debug")
    message(STATUS "Set the build type to `${default_build_type}` as none was specified.")
    set(CMAKE_BUILD_TYPE ${default_build_type} CACHE STRING "Choose the build type." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND ENGINE_COMPILE_DEFINITIONS ENABLE_ASSERTS=1)
    if(MSVC)
        list(APPEND ENGINE_COMPILE_OPTIONS
            /W4)
    else()
        list(APPEND ENGINE_COMPILE_OPTIONS
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wno-sign-conversion)
    endif()
endif()

set(DEFAULTS_LOADED_ TRUE)
