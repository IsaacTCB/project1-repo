#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

struct PcmAudioSource {
    std::span<float> pcm;
    size_t channels;
    size_t sample_rate;
};

class AudioMixer {
public:
    static constexpr size_t MAX_VOICES = 32;

public:
    void create_voice(PcmAudioSource &audio_source);
    void render_frames(std::span<float> out, size_t frames, size_t freq);

private:
    struct VoicePlaybackState {
        PcmAudioSource *audio_source;
        uint64_t frame_count{};
        uint64_t position{};
    };
    std::array<VoicePlaybackState, MAX_VOICES> m_voices{};

private:
    void end_voice(VoicePlaybackState &voice);
    VoicePlaybackState *find_free_voice();
};
