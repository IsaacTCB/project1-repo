#include "audio_mixer.h"

#include "audio_limits.h"
#include "voice_controls.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>

#include "core/math/base.h"

static constexpr uint32_t FIXED_POINT_BITS = 24;
static constexpr uint64_t FIXED_POINT_ONE = 1ull << FIXED_POINT_BITS;
static constexpr uint64_t FIXED_POINT_MASK = FIXED_POINT_ONE - 1;

AudioMixer::AudioMixer() {
    m_voices = std::make_unique<std::array<VoicePlaybackState, AudioLimits::MAX_VOICES>>();
}

static inline float approach(float from, float to, float max_delta) {
    const float delta = to - from;
    if (std::abs(delta) <= max_delta) {
        return to;
    }
    return from + std::copysign(max_delta, delta);
}

struct ResamplePcmDesc {
    /* scratch buffer */
    const float *base;
    size_t start;
    size_t capacity;
    uint64_t valid_frames; /* how many frames is ready for consumption? */
    bool eof;

    /* position */
    uint64_t &position;
    uint64_t step;

    /* output */
    float *dst;
    size_t dst_frames;

    /* gain, volume, etc. */
    float &gain_l;
    float &gain_r;
    float gain_target_l;
    float gain_target_r;
    float gain_step;
};

template <size_t SrcChannels>
static inline void mix_frame(const ResamplePcmDesc &desc, uint64_t position, float gain_l, float gain_r, float *out) {
    const auto idx = static_cast<size_t>(position >> FIXED_POINT_BITS);
    const bool has_next = (idx + 1) < desc.valid_frames;

    const size_t i0 = (desc.start + idx) % desc.capacity;
    const size_t i1 = has_next ? ((i0 + 1) % desc.capacity) : i0;

    const float *s0 = desc.base + (i0 * SrcChannels);
    const float *s1 = desc.base + (i1 * SrcChannels);

    const auto t = static_cast<float>(position & FIXED_POINT_MASK) *
                   (1.0f / static_cast<float>(FIXED_POINT_ONE));

    if constexpr (SrcChannels == 1) {
        const float sample = (s0[0] + ((s1[0] - s0[0]) * t));
        out[0] += sample * gain_l;
        out[1] += sample * gain_r;
    } else {
        out[0] += (s0[0] + ((s1[0] - s0[0]) * t)) * gain_l;
        out[1] += (s0[1] + ((s1[1] - s0[1]) * t)) * gain_r;
    }
}

template <size_t SrcChannels>
static size_t resample_pcm(const ResamplePcmDesc &desc) {
    size_t produced = 0;

    uint64_t last = desc.valid_frames;
    if (!desc.eof && last > 0) {
        last--;
    }

    /* load all variables into stack to avoid pointer jumping */
    uint64_t position = desc.position;
    float gain_l = desc.gain_l;
    float gain_r = desc.gain_r;

    const uint64_t end_position = last << FIXED_POINT_BITS;
    if (position >= end_position) {
        return 0;
    }

    const uint64_t available = ((end_position - 1 - position) / desc.step) + 1;
    const auto frames = static_cast<size_t>(std::min<uint64_t>(available, desc.dst_frames));

    /* special loop for when volume ramps are active */
    for (; produced < frames && (gain_l != desc.gain_target_l || gain_r != desc.gain_target_r); ++produced) {
        gain_l = approach(gain_l, desc.gain_target_l, desc.gain_step);
        gain_r = approach(gain_r, desc.gain_target_r, desc.gain_step);
        float *out = desc.dst + (produced * AudioLimits::OUTPUT_CHANNELS);
        mix_frame<SrcChannels>(desc, position, gain_l, gain_r, out);
        position += desc.step;
    }

    /* fast case scenario: we aren't processing a volume ramp */
    for (; produced < frames; ++produced) {
        float *out = desc.dst + (produced * AudioLimits::OUTPUT_CHANNELS);
        mix_frame<SrcChannels>(desc, position, gain_l, gain_r, out);
        position += desc.step;
    }

    /* commit variables */
    desc.gain_l = gain_l;
    desc.gain_r = gain_r;
    desc.position = position;

    return produced;
}

static uint64_t get_step_rate(size_t src_freq, float pitch = 1.0f) {
    double step_f =
        static_cast<double>(src_freq) /
        static_cast<double>(AudioLimits::OUTPUT_SAMPLE_RATE);

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

void AudioMixer::create_voice(VoiceCreateDesc &&desc) {
    auto *v = find_free_voice();

    if (!v) {
        /* Failed to find or steal a voice candidate, so we must
         * flag voice controller that this voice won't play */
        m_voice_controller->mark_finished(desc.handle_index);
        return;
    }

    const auto info = desc.instance->source()->info();
    reset_voice(*v);
    v->instance = desc.instance;
    v->handle_index = desc.handle_index;
    v->channels = info.channels;
    v->sample_rate = info.sample_rate;
}

static void fill_scratch(AudioMixer::VoicePlaybackState &v, bool looping) {
    if (v.instance->at_loop_end()) {
        v.instance->resolve_loop(looping);
    }

    while (!v.eof && v.valid < AudioMixer::SCRATCH_FRAMES) {
        const size_t write_pos = (v.start + v.valid) % AudioMixer::SCRATCH_FRAMES;
        const size_t contiguous = std::min<size_t>(
            AudioMixer::SCRATCH_FRAMES - v.valid,
            AudioMixer::SCRATCH_FRAMES - write_pos);

        const size_t got = v.instance->read(
            std::span(v.scratch.data() + (write_pos * v.channels), contiguous * v.channels));

        if (got == 0) {
            if (!v.instance->at_loop_end()) {
                v.eof = true;
            }
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

void AudioMixer::render_frames(std::span<float> out, size_t frames) {
    std::fill_n(out.begin(), frames * AudioLimits::OUTPUT_CHANNELS, 0.0f);

    for (auto &v : *m_voices) {
        if (!v.instance) {
            continue;
        }

        const auto params = m_voice_controller->params(v.handle_index);
        if (params.stop_requested) {
            v.stopping = true;
        }

        /* define gain values */
        const float gain_target_l = v.stopping ? 0.0f : params.volume;
        const float gain_target_r = v.stopping ? 0.0f : params.volume;
        if (v.gain_init) {
            v.gain_l = gain_target_l;
            v.gain_r = gain_target_r;
            v.gain_init = false;
        }

        if (v.stopping && is_zero_approx(v.gain_l) && is_zero_approx(v.gain_r)) {
            end_voice(v);
            continue;
        }

        const uint64_t step = get_step_rate(v.sample_rate, params.pitch);

        size_t produced = 0;
        while (produced < frames) {
            /* fill entire scratch buffer only when needed it */
            if (v.valid <= (frames - produced)) {
                fill_scratch(v, params.looping);
            }

            float *dst = out.data() + (produced * 2);

            const ResamplePcmDesc resample_desc{
                /* scratch buffer */
                .base = v.scratch.data(),
                .start = v.start,
                .capacity = SCRATCH_FRAMES,
                .valid_frames = v.valid,
                .eof = v.eof,

                /* position */
                .position = v.position,
                .step = step,

                /* output */
                .dst = dst,
                .dst_frames = frames - produced,

                /* gain, volume, etc. */
                .gain_l = v.gain_l,
                .gain_r = v.gain_r,
                .gain_target_l = gain_target_l,
                .gain_target_r = gain_target_r,
                .gain_step = VOLUME_GAIN_STEP,
            };

            size_t n = 0;
            switch (v.channels) {
                case 1:
                    n = resample_pcm<1>(resample_desc);
                    break;
                case 2:
                    n = resample_pcm<2>(resample_desc);
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
    m_voice_controller->mark_finished(voice.handle_index);
    reset_voice(voice);
}

void AudioMixer::reset_voice(VoicePlaybackState &voice) {
    voice.instance = nullptr;
    voice.handle_index = 0;
    voice.eof = false;
    voice.start = 0;
    voice.valid = 0;
    voice.position = 0;
    voice.channels = 0;
    voice.sample_rate = 0;
    voice.gain_l = 0.0f;
    voice.gain_r = 0.0f;
    voice.gain_init = false;
    voice.stopping = false;
}
