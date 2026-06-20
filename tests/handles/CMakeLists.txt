set(TESTS
    test_handles
)

foreach(test ${TESTS})
    create_test(${test})
    target_link_libraries(${test} PRIVATE engine)
endforeach()

