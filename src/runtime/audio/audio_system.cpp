#include "audio_system.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include "core/logger.h"
#include "runtime/audio/audio_mixer.h"

#include <vorbis/vorbisfile.h>

static PcmAudioSource s_audio_clip;
static std::vector<float> s_pcm_data;

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

    OggVorbis_File vf;
    if (ov_fopen("audio.ogg", &vf) < 0) {
        LOG_CRITICAL("Failed to open 'audio.ogg'");
        exit(2);
    }

    s_audio_clip.channels = vf.vi->channels;
    s_audio_clip.sample_rate = vf.vi->rate;

    uint64_t sample_count = ov_pcm_total(&vf, -1);
    s_pcm_data.resize(sample_count * s_audio_clip.channels);

    s_audio_clip.pcm = std::span{s_pcm_data};

    uint64_t samples_written = 0;
    while (samples_written < sample_count) {
        float **pcm = nullptr;
        int bitstream = 0;
        const int requested = static_cast<int>(std::min<uint64_t>(sample_count - samples_written, 4096));
        const long result = ov_read_float(&vf, &pcm, requested, &bitstream);

        if (result == 0) {
            break; /* EOF */
        }
        if (result < 0) {
            break;
        }
        if (static_cast<uint64_t>(bitstream) != 0) {
            break;
        }

        for (long frame = 0; frame < result; ++frame) {
            for (uint64_t channel = 0; channel < s_audio_clip.channels; ++channel) {
                s_audio_clip.pcm[((samples_written + static_cast<uint64_t>(frame)) * s_audio_clip.channels) + channel] = pcm[channel][frame];
            }
        }

        samples_written += static_cast<uint64_t>(result);
    }

    m_ad->start();

    m_mixer.create_voice(s_audio_clip);
}

void AudioSystem::close() {
    m_ad->close();
    m_ad.reset();
}

std::span<float> AudioSystem::render_audio(size_t requested_frames) {
    const auto frames = std::min(requested_frames, FRAME_SIZE);
    std::fill_n(m_buffer.begin(), frames * CHANNELS, 0.0f);

    m_mixer.render_frames(m_buffer, frames, SAMPLE_RATE);

    /* after mixing is done, apply hard-limiter */
    for (auto &sample : m_buffer) {
        sample = std::clamp(sample, -1.0f, 1.0f);
    }

    return std::span{m_buffer}.first(frames * CHANNELS);
}
