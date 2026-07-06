include(engine_utils)

option(ENABLE_PCG32 "Uses PCG32 as the primary PRNG algorithm" ON)

set(CORE_SOURCES
    logger.cpp
    math/random.cpp
    math/vec2.cpp
    math/vec2i.cpp
    math/vec3.cpp
    math/vec3i.cpp
)

add_library(core STATIC ${CORE_SOURCES})
engine_add_module(core)


# Pseudorandom number generator (PCG family)
target_compile_definitions(core PRIVATE $<$<BOOL:${ENABLE_PCG32}>:ENABLE_PCG32>)

if(ENABLE_PCG32)
    add_subdirectory(${CMAKE_SOURCE_DIR}/external/pcg-cpp external/pcg-cpp)
    target_link_libraries(core PRIVATE pcg-cpp)
endif()
