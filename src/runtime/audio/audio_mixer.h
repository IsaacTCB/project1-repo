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
    int priority;
};

class AudioMixer {
public:
    AudioMixer();

    static constexpr size_t SCRATCH_FRAMES = 4 * 512 * 2;
    static constexpr float VOLUME_RAMP_SECONDS = 0.010f;
    static constexpr float VOLUME_GAIN_STEP =
        1.0f / (VOLUME_RAMP_SECONDS * static_cast<float>(AudioLimits::OUTPUT_SAMPLE_RATE));

public:
    void set_voice_controller(IVoiceControls &voice_controller) {
        m_voice_controller = &voice_controller;
    }

    void create_voice(VoiceCreateDesc &&desc);
    void render_frames(std::span<float> out, size_t frames);

    /* NOTE: if adding a new attribute here, make sure to also update
     * ::reset_voice so that it gets reset properly. */
    struct VoicePlaybackState {
        IAudioSourceInstance *instance = nullptr;

        /* These are used for voice stealing */
        int priority;
        size_t serial;

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
    /* voice pool, contains to account for volume ramp delay when stealing */
    std::unique_ptr<std::array<VoicePlaybackState, AudioLimits::MAX_SLOTS>> m_voices;
    IVoiceControls *m_voice_controller{};
    size_t m_serial_last{}; /* used to find the oldest voice steal candidate */

private:
    /* Returns the amount of audible voices (voices that are currently
     * playing and not in the process of stopping (ramp down curve) */
    size_t audible_voices_count() const;

    /**
     * @brief Finds a suitable candidate victim for voice stealing.
     *
     * @param priority Priority used for searching candidates.
     * @return The steal candidate, or nullptr if none was found.
     */
    VoicePlaybackState *find_voice_victim(int desired_priority);

    void end_voice(VoicePlaybackState &voice);
    /* Clears entire VoicePlaybackState struct without refilling
     * scratch buffer with zeroes. Maybe if we remove the per-voice
     * buffer entirely we could also remove this, too. */
    void reset_voice(VoicePlaybackState &voice);
    VoicePlaybackState *find_free_voice();
};
