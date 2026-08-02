include(engine_utils)

function(engine_add_platform target)
    engine_add_module(${target})
    target_link_libraries(${target}
        PRIVATE
        engine::core
        engine::runtime
    )
endfunction()

# Used by platform projects to add renderers.
function(engine_add_renderer target name)
    set(renderer_target renderer_${name})

    # Add renderer subdirectory
    add_subdirectory(
        ${PROJECT_SOURCE_DIR}/src/renderer/backends/${name}
        ${CMAKE_CURRENT_BINARY_DIR}/${renderer_target}
    )

    engine_add_module(${renderer_target})

    # Link renderer with platform
    target_link_libraries(${target}
        PRIVATE
        ${renderer_target}
    )
    message(STATUS "Linked renderer ${name} to ${target}")

    if(ARGN)
        target_sources(${renderer_target} PRIVATE ${ARGN}) # Hacky, I guess.
    endif()
endfunction()
