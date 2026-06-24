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

set(OPTIONS_LOADED_ TRUE)
