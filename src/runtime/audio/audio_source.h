#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

struct AudioSourceInfo {
    size_t channels;
    size_t sample_rate;
    size_t frame_count;

    [[nodiscard]]
    double duration_seconds() const {
        return sample_rate > 0 ? static_cast<double>(frame_count) / static_cast<double>(sample_rate) : 0.0;
    }
};

struct AudioLoopPoints {
    size_t begin = 0;
    size_t end = 0; /* end point of 0 == source frame count */
};

class IAudioSource : public std::enable_shared_from_this<IAudioSource> {
public:
    /* TODO: allow IAudioSource specify loop points, which
     * will then get inherited by audio source instance */

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

    virtual void set_loop_points(const AudioLoopPoints &loop_points) = 0;

    [[nodiscard]]
    virtual bool at_loop_end() const = 0;
    virtual void resolve_loop(bool wrap) = 0;
};
