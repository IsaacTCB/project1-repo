include(engine_utils)

set(CORE_SOURCES
    logger.cpp
    math/vec2.cpp
    math/vec2i.cpp
    math/vec3.cpp
    math/vec3i.cpp
)

add_library(core STATIC ${CORE_SOURCES})
engine_add_module(core)
