set(TEST_SHARED_LIBRARIES core runtime)

add_library(audio_test_libs STATIC fakes/audio_source_signal.cpp)
target_link_libraries(audio_test_libs PRIVATE ${TEST_SHARED_LIBRARIES})

set(TESTS
    test_audio_controller
    test_audio_mixer
    test_spsc_queue
)

foreach(test ${TESTS})
    create_test(${test})
    target_link_libraries(${test} PRIVATE ${TEST_SHARED_LIBRARIES} audio_test_libs)
endforeach()

