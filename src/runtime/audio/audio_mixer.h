#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "audio_source.h"

struct VoiceCreateDesc {
    std::shared_ptr<IAudioSource> source;
};

class AudioMixer {
public:
    AudioMixer();

    static constexpr size_t MAX_VOICES = 32;
    static constexpr size_t SCRATCH_FRAMES = 4 * 512 * 2;

public:
    void create_voice(const VoiceCreateDesc &desc);
    void render_frames(std::span<float> out, size_t frames, size_t freq);

    struct VoicePlaybackState {
        std::unique_ptr<IAudioSourceInstance> instance{nullptr};

        /* scratch ring/circular buffer, used for resampling */
        std::array<float, SCRATCH_FRAMES * 2> scratch{};

        /* have we reached the end of this audio source? */
        bool eof{};

        /* start position in scratch ring buffer */
        size_t start{};

        /* how many valid frames are loaded */
        size_t valid{};

        /* cursor position relative to start */
        uint64_t position{};

        size_t channels{};
        size_t sample_rate{};
    };

private:
    std::unique_ptr<std::array<VoicePlaybackState, MAX_VOICES>> m_voices;

private:
    void end_voice(VoicePlaybackState &voice);
    VoicePlaybackState *find_free_voice();
};
