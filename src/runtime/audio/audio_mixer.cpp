#include "audio_mixer.h"

#include "core/logger.h"
#include "runtime/audio/audio_limits.h"
#include "runtime/audio/voice_controls.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

static constexpr uint32_t FIXED_POINT_BITS = 24;
static constexpr uint64_t FIXED_POINT_ONE = 1ull << FIXED_POINT_BITS;
static constexpr uint64_t FIXED_POINT_MASK = FIXED_POINT_ONE - 1;

AudioMixer::AudioMixer() {
    m_voices = std::make_unique<std::array<VoicePlaybackState, AudioLimits::MAX_VOICES>>();
}

template <size_t SrcChannels>
static size_t resample_pcm(const float *base, size_t start, size_t capacity, uint64_t valid_frames, bool eof, uint64_t &position, uint64_t step, float *dst, size_t dst_frames, float gain) {
    size_t produced = 0;
    for (; produced < dst_frames; ++produced) {
        const auto idx = static_cast<size_t>(position >> FIXED_POINT_BITS);
        if (idx >= valid_frames) {
            break;
        }
        if (!eof && idx + 1 >= valid_frames) {
            break;
        }
        const bool has_next = (idx + 1) < valid_frames;

        const float *s0 = base + (((start + idx) % capacity) * SrcChannels);
        const float *s1 = has_next
                            ? base + ((((start + idx) + 1) % capacity) * SrcChannels)
                            : s0;

        const auto t = static_cast<float>(position & FIXED_POINT_MASK) /
                       static_cast<float>(FIXED_POINT_ONE);

        float *out = dst + (produced * 2);
        if constexpr (SrcChannels == 1) {
            const float sample = (s0[0] + ((s1[0] - s0[0]) * t)) * gain;
            out[0] += sample;
            out[1] += sample;
        } else {
            out[0] += (s0[0] + ((s1[0] - s0[0]) * t)) * gain;
            out[1] += (s0[1] + ((s1[1] - s0[1]) * t)) * gain;
        }

        position += step;
    }

    return produced;
}

static uint64_t get_step_rate(size_t src_freq, size_t dst_freq, float pitch = 1.0f) {
    double step_f =
        static_cast<double>(src_freq) /
        static_cast<double>(dst_freq);

    step_f *= pitch;

    const auto step = static_cast<uint64_t>(
        step_f * static_cast<double>(FIXED_POINT_ONE));

    return std::max(step, uint64_t{1});
}

AudioMixer::VoicePlaybackState *AudioMixer::find_free_voice() {
    for (auto &voice : *m_voices) {
        if (voice.instance) {
            continue;
        }
        return &voice;
    }
    return nullptr;
}

bool AudioMixer::create_voice(const VoiceCreateDesc &desc) {
    auto *v = find_free_voice();

    if (!v) {
        return false;
    }

    const auto info = desc.source->info();
    *v = {
        .instance = desc.source->create_instance(),
        .handle_index = desc.handle_index,
        .position = 0,
        .channels = info.channels,
        .sample_rate = info.sample_rate,
    };
    return true;
}

static void fill_scratch(AudioMixer::VoicePlaybackState &v) {
    while (!v.eof && v.valid < AudioMixer::SCRATCH_FRAMES) {
        const size_t write_pos = (v.start + v.valid) % AudioMixer::SCRATCH_FRAMES;
        const size_t contiguous = std::min<size_t>(
            AudioMixer::SCRATCH_FRAMES - v.valid,
            AudioMixer::SCRATCH_FRAMES - write_pos);

        const size_t got = v.instance->read(
            std::span(v.scratch.data() + (write_pos * v.channels), contiguous * v.channels));

        if (got == 0) {
            v.eof = true;
            break;
        }

        v.valid += got;
    }
}

static void drop_consumed(AudioMixer::VoicePlaybackState &v) {
    const size_t drop = std::min<size_t>(v.position >> FIXED_POINT_BITS, v.valid);
    if (!drop) {
        return;
    }
    v.start = (v.start + drop) % AudioMixer::SCRATCH_FRAMES;
    v.valid -= drop;
    v.position -= uint64_t{drop} << FIXED_POINT_BITS;
}

void AudioMixer::render_frames(std::span<float> out, size_t frames, size_t freq) {
    std::fill_n(out.begin(), frames * AudioLimits::OUTPUT_CHANNELS, 0.0f);

    for (auto &v : *m_voices) {
        if (!v.instance) {
            continue;
        }

        const auto params = m_voice_controller->params(v.handle_index);

        if (params.stop_requested) {
            LOG_TRACE("damn");
            end_voice(v);
            continue;
        }

        const uint64_t step = get_step_rate(v.sample_rate, freq, params.pitch);

        size_t produced = 0;
        while (produced < frames) {
            /* fill entire scratch buffer only when needed it */
            if (v.valid <= (frames - produced)) {
                fill_scratch(v);
            }

            float *dst = out.data() + (produced * 2);

            size_t n = 0;
            switch (v.channels) {
                case 1:
                    n = resample_pcm<1>(
                        v.scratch.data(), v.start, SCRATCH_FRAMES,
                        v.valid, v.eof, v.position, step, dst, frames - produced,
                        params.volume);
                    break;
                case 2:
                    n = resample_pcm<2>(
                        v.scratch.data(), v.start, SCRATCH_FRAMES,
                        v.valid, v.eof, v.position, step, dst, frames - produced,
                        params.volume);
                    break;
                default:
                    break;
            }

            produced += n;
            drop_consumed(v);

            if (n == 0 && v.eof) {
                break;
            }
        }

        if (produced < frames) {
            end_voice(v);
        }
    }
}

void AudioMixer::end_voice(VoicePlaybackState &voice) {
    voice.position = 0;
    voice.start = 0;
    voice.valid = 0;

    voice.instance.reset();
    m_voice_controller->mark_finished(voice.handle_index);
}
