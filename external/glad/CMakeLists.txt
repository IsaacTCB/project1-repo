project(glad LANGUAGES C)

add_library(glad src/gl.c)
set_target_properties(glad PROPERTIES
                      C_LANGUAGE 11)

target_include_directories(glad PUBLIC include)
