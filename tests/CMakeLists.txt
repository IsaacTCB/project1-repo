cmake_minimum_required(VERSION 3.16)

enable_testing()
find_package(GTest REQUIRED)

function(create_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE GTest::gtest_main)
    add_test(NAME ${name} COMMAND $<TARGET_FILE:${name}>)
endfunction()

# Math tests
add_subdirectory(math)

