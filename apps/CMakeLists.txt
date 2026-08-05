include(engine_utils)

set(EXAMPLES
    sandbox
    platformer_2d
)

foreach(example ${EXAMPLES})
    set(EXAMPLE_NAME ${example})
    add_subdirectory(${EXAMPLE_NAME})
endforeach()
