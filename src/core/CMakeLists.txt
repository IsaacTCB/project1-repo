set(CORE_SOURCES
    logger.cpp
    math/vec2.cpp
    math/vec2i.cpp
    math/vec3.cpp
    math/vec3i.cpp
)

add_library(core STATIC ${CORE_SOURCES})
target_include_directories(core PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(core PUBLIC ${ENGINE_COMPILE_DEFINITIONS})

target_compile_options(core PRIVATE ${ENGINE_COMPILE_OPTIONS})
