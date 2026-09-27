#include "audio_source_decoded.h"

#include "audio_source.h"

#include <algorithm>
#include <cstdint>
#include <vorbis/vorbisfile.h>

#include "core/logger.h"

/* ================== *
 *       Source       *
 * ================== */

std::shared_ptr<AudioSourceDecoded> AudioSourceDecoded::open(std::unique_ptr<IAudioDecoder> decoder) {
    auto source = std::make_shared<AudioSourceDecoded>(PrivateTag{});
    if (!source->decode(std::move(decoder))) {
        return {};
    }
    return source;
}

bool AudioSourceDecoded::decode(std::unique_ptr<IAudioDecoder> decoder) {
    if (!decoder->open()) {
        return false;
    }

    const auto &info = decoder->info();
    m_info = {
        .channels = info.channels,
        .sample_rate = info.sample_rate,
        .frame_count = info.frame_count,
    };

    const size_t sample_count = m_info.frame_count * m_info.channels;
    m_pcm.resize(sample_count);

    /* decode up until frames_remaining or an error happens */
    uint64_t frames_remaining = m_info.frame_count;
    while (frames_remaining > 0) {
        const uint64_t offset =
            (m_info.frame_count - frames_remaining) * m_info.channels;

        const uint64_t frames = decoder->read_frames(
            std::span{
                m_pcm.begin() + offset,
                m_pcm.size() - offset,
            },
            frames_remaining);

        if (frames == 0) {
            /* either EOF, decoder error or format error */
            break;
        }

        frames_remaining -= frames;
    }

    /* the reported length can be slightly off, keep whatever was actually decoded */
    if (frames_remaining > 0) {
        m_info.frame_count -= frames_remaining;
        m_pcm.resize(m_info.frame_count * m_info.channels);
        m_pcm.shrink_to_fit();
        LOG_TRACE("stripped {} bytes from pcm", frames_remaining * m_info.channels * sizeof(float));
    }
    return m_info.frame_count > 0 && m_info.channels > 0;
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
    : m_source(source) {
    set_loop_points({.begin = 0, .end = source->m_info.frame_count});
}

uint32_t AudioSourceDecodedInstance::read(std::span<float> output) {
    if (m_at_loop_end) {
        return 0;
    }

    size_t channels = m_source->m_info.channels;

    const bool stop_at_loop = m_has_loop && m_position < m_loop_points.end;
    const uint64_t end = stop_at_loop ? m_loop_points.end : m_source->m_info.frame_count;

    if (m_position >= end) {
        return 0; /* eof */
    }

    uint64_t frames_requested = output.size() / channels;
    uint64_t frames_to_read = std::min(frames_requested, end - m_position);

    /* copy pcm samples */
    const auto pcm = m_source->pcm();
    std::copy_n(pcm.data() + (m_position * channels), frames_to_read * channels, output.data());
    m_position += frames_to_read;

    if (stop_at_loop && m_position == m_loop_points.end) {
        m_at_loop_end = true;
    }

    return static_cast<uint32_t>(frames_to_read);
}

bool AudioSourceDecodedInstance::seek(uint64_t frame) {
    if (frame > m_source->m_info.frame_count) {
        return false;
    }

    m_position = frame;
    return true;
}

uint64_t AudioSourceDecodedInstance::position() {
    return m_position;
}

void AudioSourceDecodedInstance::resolve_loop(bool wrap) {
    m_at_loop_end = false;
    if (wrap) {
        m_position = m_loop_points.begin;
    }
}
