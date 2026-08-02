set(TESTS
    test_color
    test_vec2
    test_vec2i
    test_vec3
    test_vec3i
)

foreach(test ${TESTS})
    create_test(${test})
    target_link_libraries(${test} PRIVATE core)
endforeach()

