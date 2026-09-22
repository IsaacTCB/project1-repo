#include "audio_system.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <span>

#include "audio_mixer.h"
#include "audio_source_decoded.h"
#include "spsc_queue.h"

AudioSystem::AudioSystem(std::unique_ptr<IAudioDevice> ad)
    : m_ad(std::move(ad)) {}

AudioSystem::~AudioSystem() {
    if (m_ad) {
        close();
    }
}

void AudioSystem::init() {
    m_ad->open({
        .format = AudioFormat::F32,
        .sample_rate = SAMPLE_RATE,
        .channels = CHANNELS,
    });

    m_ad->set_device_callback(
        AudioSystem::audio_device_callback,
        this);

    m_ad->start();

    auto source = std::static_pointer_cast<IAudioSource>(
        std::make_shared<AudioSourceDecoded>("audio.ogg"));

    m_audio_thread.commands.try_emplace(PlaySoundCommand{
        .source = std::move(source),
    });
}

void AudioSystem::close() {
    m_ad->close();
    m_ad.reset();
}

std::span<float> AudioSystem::render_audio(size_t requested_frames) {
    /* process spsc commands */
    PlaySoundCommand cmd;
    while (m_audio_thread.commands.try_pop(cmd)) {
        m_audio_thread.mixer.create_voice({
            .source = cmd.source,
        });
    }

    const auto frames = std::min(
        requested_frames, FRAME_SIZE);

    /* mix all voices into buffer */
    m_audio_thread.mixer.render_frames(
        m_buffer, frames, SAMPLE_RATE);

    /* after mixing is done, apply hard-limiter */
    for (auto &sample : m_buffer) {
        sample = std::clamp(sample, -1.0f, 1.0f);
    }

    return std::span{m_buffer}.first(frames * CHANNELS);
}
