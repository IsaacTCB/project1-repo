#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

struct AudioInfo {
    size_t channels;
    size_t sample_rate;
    size_t frame_count;
};

class IAudioDecoder {
protected:
    explicit IAudioDecoder() = default;

public:
    virtual ~IAudioDecoder() = default;
    IAudioDecoder(const IAudioDecoder &other) = delete;
    IAudioDecoder &operator=(IAudioDecoder &rhs) = delete;
    IAudioDecoder(IAudioDecoder &&other) = delete;
    IAudioDecoder &operator=(IAudioDecoder &&rhs) = delete;

    virtual bool open() = 0;

    [[nodiscard]]
    virtual const AudioInfo &info() const = 0;

    /**
     * @brief Reads up to frame_count of interleaved float PCM into out
     *
     * out.size() must be >= frame_count * info().channels.
     *
     * @param out Destination buffer
     * @param frame_count Max number of frames to be read
     *
     * @return Total amount of frames read
     */
    virtual uint64_t read_frames(std::span<float> out, uint64_t frame_count) = 0;

    virtual void seek(uint64_t offset) = 0;
    [[nodiscard]] virtual uint64_t tell() = 0;
};
