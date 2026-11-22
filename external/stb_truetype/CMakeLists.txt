cmake_minimum_required(VERSION 3.11)

project(stb_truetype LANGUAGES C)

add_library(stb_truetype STATIC src/stb_truetype.c)
target_include_directories(stb_truetype PUBLIC include)

