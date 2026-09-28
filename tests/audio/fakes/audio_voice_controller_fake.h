#pragma once

#include <array>
#include <cstdint>
#include <expected>
#include <memory>

#include "runtime/audio/audio_limits.h"
#include "runtime/audio/audio_source.h"
#include "runtime/audio/voice_controls.h"

enum class TestVoiceError : uint8_t {
    CLAIM_FAILED,
};

class TestVoiceControls : public IVoiceControls {
public:
    [[nodiscard]]
    VoiceParams params([[maybe_unused]] size_t idx) const override {
        return {
            .stop_requested = m_stop_requested,
            .volume = m_volume,
            .pitch = 1.0f,
            .looping = m_looping,
        };
    }

    void set_volume(float volume) {
        m_volume = volume;
    }

    void set_looping(bool looping) {
        m_looping = looping;
    }

    void set_stop_request(bool to_stop) {
        m_stop_requested = to_stop;
    }

    void release_claim(size_t idx) {
        m_instances[idx] = {};
        m_marked_finished[idx] = false;
    }

    void mark_finished(size_t idx) override {
        m_instances[idx].reset();
        m_marked_finished[idx] = true;
    }

    bool was_marked_finished(size_t idx) {
        return m_marked_finished[idx];
    }

    std::expected<size_t, TestVoiceError> claim() {
        for (size_t i = 0; i < m_instances.size(); ++i) {
            auto &inst = m_instances[i];
            if (inst) {
                continue;
            }

            return i;
        }

        return std::unexpected{TestVoiceError::CLAIM_FAILED};
    }

    [[nodiscard]]
    IAudioSourceInstance *attach(size_t idx, std::unique_ptr<IAudioSourceInstance> instance) {
        auto *inst = instance.get();
        m_instances[idx] = std::move(instance);
        return inst;
    }

private:
    std::array<std::unique_ptr<IAudioSourceInstance>, AudioLimits::MAX_VOICES> m_instances;
    std::array<bool, AudioLimits::MAX_VOICES> m_marked_finished;
    bool m_looping = false;
    bool m_stop_requested = false;
    float m_volume = 1.0f;
};
