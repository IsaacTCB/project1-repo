#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>

#include "core/utils/handle_pool.h"

#include "audio_controller.h"
#include "audio_device.h"
#include "audio_handles.h"
#include "audio_limits.h"
#include "audio_mixer.h"
#include "audio_source.h"
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

    /* TODO: the audio system shouldn't be assuming filesystem
     * behavior. This function is therefore temporary and is only
     * used to implement the asset ref system */
    std::shared_ptr<IAudioSource> get_source_from_path(const std::filesystem::path &filepath);

    AudioSourceHandle load_source(std::shared_ptr<IAudioSource> source);
    void unload_source(AudioSourceHandle handle);

    /* source queries, return empty/0 if the handle is invalid */
    [[nodiscard]]
    std::optional<AudioSourceInfo> get_source_info(AudioSourceHandle handle) const;
    [[nodiscard]]
    uint64_t get_source_length_frames(AudioSourceHandle handle) const;
    [[nodiscard]]
    double get_source_length_seconds(AudioSourceHandle handle) const;
    [[nodiscard]]
    size_t get_source_sample_rate(AudioSourceHandle handle) const;
    [[nodiscard]]
    size_t get_source_channels(AudioSourceHandle handle) const;

    VoiceHandle play_audio(AudioSourceHandle source_handle, const PlayDesc &desc = {});
    void stop_audio(VoiceHandle voice);

    [[nodiscard]]
    bool is_audio_playing(VoiceHandle voice) const;
    [[nodiscard]]
    bool is_audio_looping(VoiceHandle voice) const;

    void set_audio_volume(VoiceHandle voice, float volume);
    void set_audio_looping(VoiceHandle voice, bool looping);

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
        int priority;
        IAudioSourceInstance *instance;
    };

    struct AudioDeviceThreadState {
        AudioMixer mixer;

        /* SPSC command queue:
         * - produced by Game thread.
         * - consumed by Audio thread. */
        SpscQueue<PlaySoundCommand, AudioLimits::COMMAND_QUEUE_CAPACITY> commands;
    } m_audio_thread;

    VoiceController m_voice_controller;

    HandlePool<std::shared_ptr<IAudioSource>, AudioSourceHandle> m_sources;
};
