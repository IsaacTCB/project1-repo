#include "audio_source.h"

#include <cstdint>
#include <memory>
#include <vector>

#include "audio_decoder.h"

class AudioSourceDecoded : public IAudioSource {
private:
    struct PrivateTag {};
    AudioSourceDecoded() = default;

public:
    explicit AudioSourceDecoded([[maybe_unused]] PrivateTag tag) {};

    [[nodiscard]]
    static std::shared_ptr<AudioSourceDecoded> open(std::unique_ptr<IAudioDecoder> decoder);

    AudioSourceInfo info() const override;
    std::unique_ptr<IAudioSourceInstance> create_instance() override;

protected:
    std::span<const float> pcm() const { return std::span{m_pcm}; }
    friend class AudioSourceDecodedInstance;

private:
    bool decode(std::unique_ptr<IAudioDecoder> decoder);

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

    void set_loop_points(const AudioLoopPoints &loop_points) override {
        const uint64_t frame_count = m_source->m_info.frame_count;
        const uint64_t end = loop_points.end == 0 ? frame_count : std::min<uint64_t>(loop_points.end, frame_count);
        const uint64_t begin = std::min<uint64_t>(loop_points.begin, end);

        m_loop_points = {.begin = begin, .end = end};
        m_has_loop = begin < end;
    }

    [[nodiscard]]
    bool at_loop_end() const override { return m_at_loop_end; }
    void resolve_loop(bool wrap) override;

private:
    void copy_pcm(std::span<float> output, std::span<float> src);

private:
    std::shared_ptr<AudioSourceDecoded> m_source;
    uint64_t m_position{0};

    bool m_has_loop = false;
    AudioLoopPoints m_loop_points{};

    bool m_at_loop_end = false;
};
