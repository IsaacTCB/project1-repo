#include "audio_source.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

class AudioSourceDecoded : public IAudioSource {
private:
    struct PrivateTag {};
    AudioSourceDecoded() = default;

public:
    explicit AudioSourceDecoded([[maybe_unused]] PrivateTag tag) {};

    [[nodiscard]]
    static std::shared_ptr<AudioSourceDecoded> open(const std::filesystem::path &path);

    bool decode(const std::filesystem::path &path);

    AudioSourceInfo info() const override;
    std::unique_ptr<IAudioSourceInstance> create_instance() override;

protected:
    std::span<const float> pcm() const { return std::span{m_pcm}; }
    friend class AudioSourceDecodedInstance;

private:
    AudioSourceInfo m_info;
    std::vector<float> m_pcm;
};

class AudioSourceDecodedInstance : public IAudioSourceInstance {
public:
    AudioSourceDecodedInstance(const std::shared_ptr<AudioSourceDecoded> &source);

    [[nodiscard]]
    IAudioSource *source() const override {
        return m_source.get();
    }

    uint32_t read(std::span<float> output) override;
    bool seek(uint64_t frame) override;
    uint64_t position() override;

private:
    void copy_pcm(std::span<float> output, std::span<float> src);

private:
    std::shared_ptr<AudioSourceDecoded> m_source;
    uint64_t m_position{0};
};
