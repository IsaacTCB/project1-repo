#include "audio_system.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <numbers>
#include <span>

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
}

void AudioSystem::close() {
    m_ad->close();
    m_ad.reset();
}

std::span<float> AudioSystem::render_audio(size_t requested_frames) {
    const size_t frames = std::min(requested_frames, FRAME_SIZE);
    std::fill_n(m_buffer.begin(), frames * CHANNELS, 0.0f);

    /* temporary: generate sine wave so that
     * we have something to play */
    size_t outframe = 0;
    for (; outframe < frames; ++outframe) {
        constexpr float SINE_FREQ = 440.0f;
        const float phase =
            static_cast<float>(m_current_size_sample) *
            SINE_FREQ / SAMPLE_RATE;

        auto *s = &m_buffer[outframe * CHANNELS];
        s[0] += std::sin(phase * 1.0f * static_cast<float>(std::numbers::pi));
        s[1] += s[0];

        m_current_size_sample++;
    }

    m_current_size_sample %= SAMPLE_RATE;

    const size_t final_size = outframe * CHANNELS;
    return std::span{m_buffer}.first(final_size);
}
