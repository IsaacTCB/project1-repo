engine_create_application(${EXAMPLE_NAME}
    src/sandbox_app.cpp
)

# Adds engine-level warnings and compile definitions
engine_add_module(${EXAMPLE_NAME})
