engine_create_application(${EXAMPLE_NAME}
    src/sandbox_app.cpp
    src/particle_system_2d.cpp
    src/particle_emitter_2d.cpp
)

set(EXAMPLE_ASSETS
    assets/test.png
)

# Adds engine-level warnings and compile definitions
engine_add_module(${EXAMPLE_NAME})

# Copy all assets after build
add_custom_command(
    TARGET ${EXAMPLE_NAME}
    POST_BUILD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${EXAMPLE_ASSETS}
            "$<TARGET_FILE_DIR:${EXAMPLE_NAME}>"
    COMMENT "${EXAMPLE_NAME}: Copying assets onto target directory"
    VERBATIM
)
