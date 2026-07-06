engine_create_application(${EXAMPLE_NAME}
    sandbox_app.cpp
)

set(EXAMPLE_ASSETS
    test.png
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
            "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "${EXAMPLE_NAME}: Copying assets onto target directory"
    VERBATIM
)
