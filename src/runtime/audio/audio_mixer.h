#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "audio_limits.h"
#include "audio_source.h"
#include "voice_controls.h"

struct VoiceCreateDesc {
    IAudioSourceInstance *instance;
    size_t handle_index;
};

class AudioMixer {
public:
    AudioMixer();

    static constexpr size_t SCRATCH_FRAMES = 4 * 512 * 2;

public:
    void set_voice_controller(IVoiceControls &voice_controller) {
        m_voice_controller = &voice_controller;
    }

    void create_voice(VoiceCreateDesc &&desc);
    void render_frames(std::span<float> out, size_t frames);

    struct VoicePlaybackState {
        IAudioSourceInstance *instance = nullptr;

        /* handle index */
        size_t handle_index = 0;

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

        /* gain & gain ramp */
        float gain_l = 0.0f;
        float gain_r = 0.0f;
        bool gain_init = false;

        /* flags */
        bool stopping = false;
    };

private:
    std::unique_ptr<std::array<VoicePlaybackState, AudioLimits::MAX_VOICES>> m_voices;
    IVoiceControls *m_voice_controller{};

private:
    void end_voice(VoicePlaybackState &voice);
    VoicePlaybackState *find_free_voice();
};
