include(engine_utils)

set(RUNTIME_SOURCES
    audio/audio_system.cpp
    audio/decoders/vorbis_audio_decoder.cpp
    audio/audio_controller.cpp
    audio/audio_source_decoded.cpp
    audio/audio_mixer.cpp
    asset_service.cpp
    input_service.cpp
    image/decoder.cpp
    renderer/renderer_2d.cpp
    renderer/batch_builder_2d.cpp
    system/view/view_system_2d.cpp
    runtime.cpp
)

add_library(runtime STATIC ${RUNTIME_SOURCES})
target_link_libraries(runtime PRIVATE core)
engine_add_module(runtime)

# stb_image
add_subdirectory(${PROJECT_SOURCE_DIR}/external/stb_image external/stb_image)
target_link_libraries(runtime PRIVATE stb_image)

# vorbis
find_package(vorbis)
if(vorbis_FOUND)
    target_link_libraries(runtime PRIVATE Vorbis::vorbisfile)
else()
    add_subdirectory(${PROJECT_SOURCE_DIR}/external/vorbis external/vorbis)
    target_link_libraries(runtime PRIVATE Vorbis::vorbisfile)
endif()

