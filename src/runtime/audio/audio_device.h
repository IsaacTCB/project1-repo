#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

enum AudioFormat : uint8_t {
    F32,
    S16,
};

struct AudioDeviceInitDesc {
    AudioFormat format = AudioFormat::F32;
    int sample_rate = 48000;
    int channels = 2;
};

class IAudioDevice {
public:
    using ByteSpan = std::span<const std::byte>;
    using CallbackFn = ByteSpan (*)(void *userdata, size_t frames);

public:
    virtual ~IAudioDevice() = default;

    virtual void open(const AudioDeviceInitDesc &desc) = 0;
    virtual void start() = 0;
    virtual void close() = 0;

public:
    void set_device_callback(CallbackFn callback, void *userdata) {
        m_callback = {
            .func = callback,
            .userdata = userdata,
        };
    }

protected:
    struct CallbackInfo {
        CallbackFn func = nullptr;
        void *userdata = nullptr;
    } m_callback{};
};
