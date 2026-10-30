macro(engine_detect_platform)
    if(TARGET_PLATFORM STREQUAL "sdl3")
        set(PLATFORM_SDL3 true)
    else()
        message(FATAL_ERROR "Unknown platform \'${TARGET_PLATFORM}\'.")
    endif()
endmacro()

function(engine_add_module target)
    target_include_directories(${target} PUBLIC ${PROJECT_SOURCE_DIR}/src)
    target_compile_options(${target} PRIVATE ${ENGINE_COMPILE_OPTIONS})
    target_compile_definitions(${target} PUBLIC ${ENGINE_COMPILE_DEFINITIONS})
endfunction()

function(engine_copy_dll target)
    if(NOT TARGET ${target})
        message(WARNING "engine_copy_dll: Target '${target}' does not exist.")
        return()
    endif()

    message(STATUS "Configuring DLL copy for target '${target}'")

    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E
        $<IF:$<BOOL:$<TARGET_RUNTIME_DLLS:${target}>>,copy_if_different,echo>
        "$<TARGET_RUNTIME_DLLS:${target}>"
        "$<TARGET_FILE_DIR:${target}>"
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
endfunction()

# Copies assets next to the target's binary after build, preserving
# their directory layout relative to CMAKE_CURRENT_SOURCE_DIR.
# e.g. 'assets/textures.png' -> '<target dir>/assets/texture.png'
function(engine_add_assets target)
    if(NOT TARGET ${target})
        message(WARNING "engine_add_assets: Target '${target}' does not exist.")
        return()
    endif()

    set(copy_commands)
    set(asset_dirs)
    foreach(asset ${ARGN})
        get_filename_component(asset_path "${asset}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        file(RELATIVE_PATH asset_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${asset_path}")
        if(asset_rel MATCHES "^\\.\\./")
            message(FATAL_ERROR "engine_add_assets: Asset '${asset}' is outside of '${CMAKE_CURRENT_SOURCE_DIR}'.")
        endif()

        if(NOT EXISTS "${asset_path}")
            message(FATAL_ERROR "engine_add_assets: Asset '${asset}' does not exist.")
        endif()

        get_filename_component(asset_dir "${asset_rel}" DIRECTORY)
        list(APPEND asset_dirs "$<TARGET_FILE_DIR:${target}>/${asset_dir}")
        list(APPEND copy_commands
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${asset_path}"
                    "$<TARGET_FILE_DIR:${target}>/${asset_rel}"
        )
    endforeach()

    if(NOT copy_commands)
        return()
    endif()
    list(REMOVE_DUPLICATES asset_dirs)

    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${asset_dirs}
        ${copy_commands}
        COMMENT "${target}: Copying assets onto target directory..."
        VERBATIM
    )
endfunction()
