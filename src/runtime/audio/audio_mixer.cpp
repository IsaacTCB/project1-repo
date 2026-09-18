#include "audio_mixer.h"
#include "core/logger.h"

#include <algorithm>
#include <cstdint>
#include <span>

static constexpr uint32_t FIXED_POINT_BITS = 24;
static constexpr uint64_t FIXED_POINT_ONE = 1ull << FIXED_POINT_BITS;
static constexpr uint64_t FIXED_POINT_MASK = FIXED_POINT_ONE - 1;

template <size_t SrcChannels>
static size_t resample_pcm(const float *src, uint64_t src_frames, uint64_t &position, uint64_t step, float *dst, size_t dst_frames) {
    const auto src_end = src_frames << FIXED_POINT_BITS;

    size_t produced = 0;
    for (; produced < dst_frames && position < src_end; ++produced) {
        const auto idx = static_cast<size_t>(position >> FIXED_POINT_BITS);

        const float *s0 = src + (idx * SrcChannels);
        float *out = dst + (produced * 2);

        if ((position & FIXED_POINT_MASK) == 0) {
            /* fast-case scenario, no interpolation */
            if constexpr (SrcChannels == 1) {
                out[0] += s0[0];
                out[1] += s0[0];
            } else {
                out[0] += s0[0];
                out[1] += s0[1];
            }
        } else {
            /* slow-ish case: linear interpolation */
            const bool has_next = (idx + 1) < src_frames;
            const float *s1 =
                has_next
                    ? (s0 + SrcChannels)
                    : s0;

            const auto t = static_cast<float>(position & FIXED_POINT_MASK) /
                           static_cast<float>(FIXED_POINT_ONE);

            if constexpr (SrcChannels == 1) {
                const float sample = s0[0] + ((s1[0] - s0[0]) * t);
                out[0] += sample;
                out[1] += sample;
            } else {
                out[0] += s0[0] + ((s1[0] - s0[0]) * t);
                out[1] += s0[1] + ((s1[1] - s0[1]) * t);
            }
        }

        position += step;
    }

    return produced;
}

static uint64_t get_step_rate(size_t src_freq, size_t dst_freq, float pitch = 1.0f) {
    float step_f =
        static_cast<float>(src_freq) /
        static_cast<float>(dst_freq);

    step_f *= pitch;

    const auto step = static_cast<uint64_t>(
        step_f * static_cast<double>(FIXED_POINT_ONE));

    return std::max(step, 1ul);
}

AudioMixer::VoicePlaybackState *AudioMixer::find_free_voice() {
    for (auto &voice : m_voices) {
        if (voice.audio_source) {
            continue;
        }
        return &voice;
    }
    return nullptr;
}

void AudioMixer::create_voice(PcmAudioSource &audio_source) {
    auto *v = find_free_voice();
    if (!v) {
        return;
    }

    const size_t frame_count =
        audio_source.pcm.size() /
        audio_source.channels;

    *v = {
        .audio_source = &audio_source,
        .frame_count = frame_count,
        .position = 0,
    };
}

void AudioMixer::render_frames(std::span<float> out, size_t frames, size_t freq) {
    for (auto &voice : m_voices) {
        if (!voice.audio_source) {
            continue;
        }

        if (voice.position >= (voice.frame_count << FIXED_POINT_BITS)) {
            end_voice(voice);
            return;
        }

        const uint64_t step = get_step_rate(voice.audio_source->sample_rate, freq);
        const float *src = voice.audio_source->pcm.data();

        size_t produced = 0;

        /* TODO: specify channel count */
        switch (2) {
            case 1:
                produced = resample_pcm<1>(
                    src, voice.frame_count, voice.position, step, out.data(), frames);
                break;
            case 2:
                produced = resample_pcm<2>(
                    src, voice.frame_count, voice.position, step, out.data(), frames);
                break;
            default:
                break;
        }

        if (produced < frames) {
            end_voice(voice);
        }
    }
}

void AudioMixer::end_voice(VoicePlaybackState &voice) {
    voice.audio_source = nullptr;
}
