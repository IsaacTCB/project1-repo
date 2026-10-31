engine_create_application(${EXAMPLE_NAME}
    src/sandbox_app.cpp
    src/sprite_manager.cpp
)

set(EXAMPLE_ASSETS
    assets/manifest.json
    assets/sprites/test.json
    assets/sprites/test.png
)

# Adds engine-level warnings and compile definitions
engine_add_module(${EXAMPLE_NAME})

add_subdirectory(${PROJECT_SOURCE_DIR}/external/glaze external/glaze)
target_link_libraries(${EXAMPLE_NAME} PRIVATE glaze::glaze)

# Copy all assets after build
engine_add_assets(${EXAMPLE_NAME} ${EXAMPLE_ASSETS})
