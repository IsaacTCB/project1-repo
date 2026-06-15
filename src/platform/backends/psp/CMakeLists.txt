cmake_minimum_required(VERSION 3.11)

if(NOT PSP)
    message(FATAL_ERROR "To target the PSP platform, you must build using the PSPSDK toolchain")
endif()

option(PSP_ENABLE_CONTROLLER_EXTENSION "If true, enables right stick and L3/R3 for PSVita or for the DualShock 3 when paired with the PSPGo" OFF)

set(PSP_SOURCES
    psp_common.cpp
    psp_display_server.cpp
    psp_io_server.cpp
    psp_os_server.cpp
    psp_platform.cpp
    psp_main.cpp
)

# FIXME: some PSPSDK header files (like pspctrl.h) uses nested anonymous structs
# for certain structures. This practice, while valid in C, is forbidden by modern
# C++ ISO standards and thus results in compiler warnings that get picked up by
# -Wpedantic. The cleanest way of "solving" the issue is to suppress them by
# disabling pedantic warnings platform-implementatation wide.
add_executable(${TARGET_NAME} ${PSP_SOURCES})

target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(${TARGET_NAME} PRIVATE
    $<$<BOOL:${PSP_ENABLE_CONTROLLER_EXTENSION}>:PSP_ENABLE_CONTROLLER_EXTENSION>
)
target_link_libraries(${TARGET_NAME}
    PRIVATE
    engine
    PUBLIC
    pspctrl
    pspdebug
    pspdisplay
)
target_add_renderer(${TARGET_NAME} pspgu)

# TODO: add a way for games/apps to specify their
# own custom assets and names.
create_pbp_file(
    TARGET ${TARGET_NAME}
    ICON_PATH ${CMAKE_CURRENT_SOURCE_DIR}/ICON0.png
    BACKGROUND_PATH NULL
    PREVIEW_PATH NULL
    TITLE ${TARGET_NAME}
)
