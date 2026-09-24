#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>
#include <vector>

#include "audio_controller.h"
#include "audio_device.h"
#include "audio_handles.h"
#include "audio_limits.h"
#include "audio_mixer.h"
#include "spsc_queue.h"

class AudioSystem {
private:
    static constexpr int CHANNELS = AudioLimits::OUTPUT_CHANNELS;
    static constexpr int SAMPLE_RATE = AudioLimits::OUTPUT_SAMPLE_RATE;

    static constexpr size_t FRAME_SIZE = 1024;
    static constexpr size_t BUFFER_SIZE = CHANNELS * FRAME_SIZE;

public:
    explicit AudioSystem(std::unique_ptr<IAudioDevice> ad);
    ~AudioSystem();

    void init();
    void close();

    void update();

    AudioSourceHandle load_source(const std::filesystem::path &filepath);
    void unload_source(AudioSourceHandle handle);

    VoiceHandle play_audio(AudioSourceHandle source, const PlayDesc &desc = {});
    void stop_audio(VoiceHandle voice);

    [[nodiscard]]
    bool is_audio_playing(VoiceHandle voice) const;

private:
    std::span<float> render_audio(size_t requested_frames);

    static IAudioDevice::ByteSpan audio_device_callback(void *userdata, size_t frames) {
        auto *audio = static_cast<AudioSystem *>(userdata);
        return std::as_bytes(audio->render_audio(frames));
    }

private:
    std::unique_ptr<IAudioDevice> m_ad;
    std::array<float, BUFFER_SIZE> m_buffer;

    struct PlaySoundCommand {
        size_t handle_index;
        std::shared_ptr<IAudioSource> source;
    };

    struct AudioDeviceThreadState {
        AudioMixer mixer;

        /* SPSC command queue:
         * - produced by Game thread.
         * - consumed by Audio thread. */
        SpscQueue<PlaySoundCommand, AudioLimits::MAX_VOICES> commands;
    } m_audio_thread;

    VoiceController m_voice_controller;

    HandlePool<std::shared_ptr<IAudioSource>> m_sources;
    std::vector<std::shared_ptr<IAudioSource>> m_unloaded;
};
