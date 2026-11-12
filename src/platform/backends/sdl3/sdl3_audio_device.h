#include "runtime/audio/audio_device.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_log.h>
#include <cstddef>

#include "core/assertion.h"

class SDL3_AudioDevice : public IAudioDevice {
public:
    void open(const AudioDeviceInitDesc &desc) override {
        ASSERT(!m_device, "Attempted to re-open device");

        const auto get_format = [](AudioFormat format) {
            switch (format) {
                case AudioFormat::F32:
                    return SDL_AUDIO_F32;
                case AudioFormat::S16:
                    return SDL_AUDIO_S16LE;
                default:
                    ASSERT(false, "Unsupported SDL3 format!");
                    return SDL_AUDIO_UNKNOWN;
            }
        };

        m_spec = {
            .format = get_format(desc.format),
            .channels = desc.channels,
            .freq = desc.sample_rate,
        };

        m_device = SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
            &m_spec,
            &SDL3_AudioDevice::callback,
            this);

        if (!m_device) {
            LOG_ERROR("sdl3_audio: failed to load audio {}", SDL_GetError());
        }
    }

    void start() override {
        ASSERT(m_device);
        SDL_ResumeAudioStreamDevice(m_device);
    }

    void close() override {
        ASSERT(m_device);

        /* close the device */
        SDL_PauseAudioStreamDevice(m_device);
        SDL_DestroyAudioStream(m_device);
        m_device = nullptr;
    }

private:
    static void callback(
        void *userdata,
        SDL_AudioStream *stream,
        int additional_amount,
        int total_amount) {
        /* fix warnings */
        (void)(total_amount);

        auto *audio = static_cast<SDL3_AudioDevice *>(userdata);
        if (!audio || !audio->m_callback.func) {
            return;
        }

        const size_t frame_size =
            SDL_AUDIO_BYTESIZE(audio->m_spec.format) *
            audio->m_spec.channels;

        size_t requested_frames =
            static_cast<size_t>(additional_amount) / frame_size;
        while (requested_frames > 0) {
            const auto pcm_buffer = audio->m_callback.func(
                audio->m_callback.userdata,
                requested_frames);

            SDL_PutAudioStreamData(
                stream,
                pcm_buffer.data(),
                static_cast<int>(pcm_buffer.size_bytes()));

            requested_frames -= std::min(
                pcm_buffer.size_bytes() / frame_size,
                requested_frames);
        }
    }

private:
    SDL_AudioSpec m_spec{};
    SDL_AudioStream *m_device = nullptr;
};
