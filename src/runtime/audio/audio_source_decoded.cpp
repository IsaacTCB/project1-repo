#include "audio_source_decoded.h"

#include "core/logger.h"
#include "runtime/audio/audio_source.h"

#include <cstdint>
#include <vorbis/vorbisfile.h>

/* ================== *
 *       Source       *
 * ================== */

AudioSourceDecoded::AudioSourceDecoded(const std::filesystem::path &path) {
    OggVorbis_File vf;
    if (ov_fopen(path.c_str(), &vf) < 0) {
        LOG_CRITICAL("Failed to open '{}'", path.string());
        exit(2);
    }

    m_info.channels = vf.vi->channels;
    m_info.sample_rate = vf.vi->rate;
    m_info.frame_count = ov_pcm_total(&vf, -1);

    const size_t sample_count = m_info.frame_count * m_info.channels;
    m_pcm.resize(sample_count);

    uint64_t frames_written = 0;
    while (frames_written < m_info.frame_count) {
        float **pcm = nullptr;
        int bitstream = 0;

        const int requested = static_cast<int>(
            std::min<uint64_t>(m_info.frame_count - frames_written, 4096));

        const long result = ov_read_float(&vf, &pcm, requested, &bitstream);

        if (result <= 0) {
            break;
        }
        if (static_cast<uint64_t>(bitstream) != 0) {
            break;
        }

        for (long frame = 0; frame < result; ++frame) {
            for (uint64_t channel = 0; channel < m_info.channels; ++channel) {
                m_pcm[((frames_written + static_cast<uint64_t>(frame)) * m_info.channels) + channel] =
                    pcm[channel][frame];
            }
        }

        frames_written += static_cast<uint64_t>(result);
    }

    ov_clear(&vf);
}

AudioSourceInfo AudioSourceDecoded::info() const {
    return m_info;
}

std::unique_ptr<IAudioSourceInstance> AudioSourceDecoded::create_instance() {
    return std::make_unique<AudioSourceDecodedInstance>(
        std::static_pointer_cast<AudioSourceDecoded>(shared_from_this()));
}

/* ================== *
 *      Instance      *
 * ================== */

AudioSourceDecodedInstance::AudioSourceDecodedInstance(const std::shared_ptr<AudioSourceDecoded> &source)
    : m_source(source) {}

uint32_t AudioSourceDecodedInstance::read(std::span<float> output) {
    auto frames_requested = static_cast<uint32_t>(output.size() / m_source->info().channels);
    auto frames_available = static_cast<uint32_t>(m_source->info().frame_count - m_position);
    uint32_t frames_to_read = std::min(frames_requested, frames_available);

    size_t channels = m_source->m_info.channels;

    /* copy pcm samples */
    const auto pcm = m_source->pcm();
    for (size_t i = 0; i < frames_to_read * channels; ++i) {
        output[i] = pcm[(m_position * channels) + i];
    }

    m_position += frames_to_read;
    return frames_to_read;
}

bool AudioSourceDecodedInstance::seek(uint64_t frame) {
    if (m_position > m_source->m_info.frame_count) {
        return false;
    }

    m_position = frame;
    return true;
}

uint64_t AudioSourceDecodedInstance::position() {
    return m_position;
}
