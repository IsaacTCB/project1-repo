#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

struct AudioSourceInfo {
    size_t channels;
    size_t sample_rate;
    size_t frame_count;
};

class IAudioSource : public std::enable_shared_from_this<IAudioSource> {
public:
    virtual ~IAudioSource() = default;

    [[nodiscard]]
    virtual AudioSourceInfo info() const = 0;

    [[nodiscard]]
    virtual std::unique_ptr<class IAudioSourceInstance> create_instance() = 0;
};

class IAudioSourceInstance {
public:
    virtual ~IAudioSourceInstance() = default;

    [[nodiscard]]
    virtual IAudioSource *source() const = 0;

    virtual uint32_t read(std::span<float> output) = 0;
    virtual bool seek(uint64_t frame) = 0;
    virtual uint64_t position() = 0;
};
