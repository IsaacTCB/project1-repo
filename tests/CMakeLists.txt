include(GoogleTest)

set(GTEST_VERSION 1.17)
set(GTEST_FETCH_VERSION 1.17.0)

find_package(GTest ${GTEST_VERSION} QUIET)
if(NOT GTest_FOUND)
    message(VERBOSE "Google Test installation was not found on system. Downloading GTest ${GTEST_VERSION}...")

    include(FetchContent)
    FetchContent_Declare("GTest"
        GIT_REPOSITORY  https://github.com/google/googletest.git
        GIT_TAG         v${GTEST_FETCH_VERSION}
        GIT_SHALLOW     TRUE
        GIT_PROGRESS    TRUE
    )
    FetchContent_MakeAvailable("GTest")
    set(GTest_FOUND TRUE)
endif()

function(create_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE GTest::gtest_main)
    gtest_discover_tests(${name})
endfunction()

# Handles tests
add_subdirectory(handles)

# Math tests
add_subdirectory(math)

