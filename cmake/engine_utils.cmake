macro(engine_detect_platform)
    if(TARGET_PLATFORM STREQUAL "sdl3")
        set(PLATFORM_SDL3 true)
    else()
        message(FATAL_ERROR "Unknown platform \'${TARGET_PLATFORM}\'.")
    endif()
endmacro()

function(engine_add_module target)
    target_include_directories(${target} PUBLIC ${CMAKE_SOURCE_DIR}/src)
    target_compile_options(${target} PRIVATE ${ENGINE_COMPILE_OPTIONS})
    target_compile_definitions(${target} PUBLIC ${ENGINE_COMPILE_DEFINITIONS})
endfunction()

function(engine_copy_dll target)

    if($<TARGET_FILE_DIR:${target}>)
        add_custom_command(
            TARGET ${target}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_RUNTIME_DLLS:${target}>"
                "$<TARGET_FILE_DIR:${target}>"
            COMMAND_EXPAND_LISTS
            VERBATIM
        )
    endif()
endfunction()
