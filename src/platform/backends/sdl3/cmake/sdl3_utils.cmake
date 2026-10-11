set(SDL3_VERSION 3.4)
set(SDL3_FETCH_VERSION 3.4.2)

macro(find_or_download_sdl3)
    include(FetchContent)
    find_package(SDL3 ${SDL3_VERSION} QUIET)
    if(NOT SDL3_FOUND)
        message(VERBOSE "SDL3 installation was not found on system. Downloading SDL3 ${SDL3_VERSION}...")

        FetchContent_Declare("SDL3"
        GIT_REPOSITORY  https://github.com/libsdl-org/SDL.git
        GIT_TAG         release-${SDL3_FETCH_VERSION}
        GIT_SHALLOW     TRUE
        GIT_PROGRESS    TRUE
    )
        FetchContent_MakeAvailable("SDL3")
        set(SDL3_FOUND TRUE)
    endif()
endmacro()

# Creates SDL3 target using ${TARGET_NAME}
macro(create_sdl3_target target_name)
    if(ANDROID)
        # Android has special target requirements:
        # it needs to be a shared library called 'main'.
        add_library(${target_name} SHARED ${ARGN})
    else()
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            add_executable(${target_name} ${ARGN})
        else()
            # For non-debug builds, disable console on Windows.
            add_executable(${target_name} WIN32 ${ARGN})
        endif()
    endif()
endmacro()

macro(fetch_gamecontrollerdb)
    set(sdl_controller_db_FILEPATH "" CACHE INTERNAL "Path to downloaded gamecontrollerdb" FORCE)

    set(_db_url "https://raw.githubusercontent.com/mdqinc/SDL_GameControllerDB/refs/heads/master/gamecontrollerdb.txt")
    set(_db_path "${CMAKE_BINARY_DIR}/gamecontrollerdb.txt")

    message(STATUS "Downloading 'gamecontrollerdb.txt'...")

    file(DOWNLOAD
        "${_db_url}"
        "${_db_path}"
        STATUS _download_status
        TIMEOUT 10
        TLS_VERIFY ON
    )

    list(GET _download_status 0 _download_code)

    if(_download_code EQUAL 0)
        set(sdl_controller_db_FILEPATH "${_db_path}" CACHE INTERNAL "Path to downloaded gamecontrollerdb" FORCE)
        message(STATUS "Downloaded 'gamecontrollerdb.txt'")
    else()
        list(GET _download_status 1 _download_message)
        message(WARNING "Could not download 'gamecontrollerdb.txt': ${_download_message}. Continuing without it.")
        file(REMOVE "${_db_path}")
    endif()

endmacro()
