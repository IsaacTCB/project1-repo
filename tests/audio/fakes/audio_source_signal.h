#pragma once

#include "runtime/audio/audio_decoder.h"

#include <cstddef>
#include <cstdint>

struct ConstSignalDesc {
    float value = 1.0f;
    size_t length = 1000;
    size_t channels = 1;
    size_t sample_rate = 48000;

    float ramp_step = 0.0f;
};

class ConstSignal_AudioDecoder : public IAudioDecoder {
public:
    ConstSignal_AudioDecoder(const ConstSignalDesc &desc);

    bool open() override;

    [[nodiscard]]
    const AudioInfo &info() const override;

    uint64_t read_frames(std::span<float> out, uint64_t frame_count) override;

    void seek(uint64_t offset) override;
    uint64_t tell() override;

private:
    ConstSignalDesc m_desc{};
    AudioInfo m_info{};
    uint64_t m_position{};
};
