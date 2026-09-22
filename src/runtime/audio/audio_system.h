#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <span>

#include "audio_device.h"
#include "audio_mixer.h"
#include "spsc_queue.h"

class AudioSystem {
private:
    static constexpr int CHANNELS = 2;
    static constexpr int SAMPLE_RATE = 48000;

    static constexpr size_t FRAME_SIZE = 1024;
    static constexpr size_t BUFFER_SIZE = CHANNELS * FRAME_SIZE;

public:
    explicit AudioSystem(std::unique_ptr<IAudioDevice> ad);
    ~AudioSystem();

    void init();
    void close();

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
        std::shared_ptr<IAudioSource> source;
    };

    struct AudioDeviceThreadState {
        AudioMixer mixer;

        /* SPSC command queue:
         * - produced by Game thread.
         * - consumed by Audio thread. */
        SpscQueue<PlaySoundCommand, 32> commands;
    } m_audio_thread;
};
