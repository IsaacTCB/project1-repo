cmake_minimum_required(3.11)

add_library(engine::platform
            STATIC
            dummy_common.cpp
            dummy_display_server.cpp
            dummy_io_server.cpp
            dummy_main.cpp
            dummy_os_server.cpp)
