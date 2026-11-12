#pragma once

#include "../audio_decoder.h"

#include <filesystem>
#include <fstream>

#include <dr_wav.h>

class WAV_AudioDecoder final : public IAudioDecoder {
public:
    WAV_AudioDecoder(const std::filesystem::path &filepath);
    ~WAV_AudioDecoder() override;

    bool open() override;

    [[nodiscard]]
    const AudioInfo &info() const override { return m_info; }

    uint64_t read_frames(std::span<float> out, uint64_t frame_count) override;

    void seek(uint64_t offset) override;
    [[nodiscard]] uint64_t tell() override;

private:
    std::ifstream m_file;
    drwav m_wav;

    AudioInfo m_info{};
    bool m_open = false;
};
