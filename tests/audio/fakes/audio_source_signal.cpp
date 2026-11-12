#include "audio_source_signal.h"

#include <algorithm>
#include <cstddef>

ConstSignal_AudioDecoder::ConstSignal_AudioDecoder(const ConstSignalDesc &desc)
    : m_desc(desc) {}

bool ConstSignal_AudioDecoder::open() {
    m_info = {
        .channels = m_desc.channels,
        .sample_rate = m_desc.sample_rate,
        .frame_count = m_desc.length,
    };

    return m_info.frame_count > 0;
};

const AudioInfo &ConstSignal_AudioDecoder::info() const {
    return m_info;
}

uint64_t ConstSignal_AudioDecoder::read_frames(std::span<float> out, uint64_t frame_count) {
    if (m_position >= m_info.frame_count) {
        return 0; /* eof */
    }

    const size_t channels = m_info.channels;
    uint64_t frames_to_read = std::min(frame_count, m_info.frame_count - m_position);

    /* copy pcm samples */
    for (size_t i = 0; i < frames_to_read; ++i) {
        const auto pos = static_cast<float>(m_position + i);
        const float s = m_desc.value + (pos * m_desc.ramp_step);

        for (size_t j = 0; j < channels; ++j) {
            out[(i * channels) + j] = s;
        }
    }
    m_position += frames_to_read;

    return frames_to_read;
}

void ConstSignal_AudioDecoder::seek(uint64_t offset) {
    if (offset >= m_info.frame_count) {
        return;
    }
    m_position = offset;
}

uint64_t ConstSignal_AudioDecoder::tell() {
    return m_position;
}
