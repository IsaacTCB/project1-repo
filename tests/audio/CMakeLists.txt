set(TESTS
    test_spsc_queue
)

foreach(test ${TESTS})
    create_test(${test})
    target_link_libraries(${test} PRIVATE core)
endforeach()

