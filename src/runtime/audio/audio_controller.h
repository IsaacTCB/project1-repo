#pragma once

#include "core/utils/handles.h"

#include <array>
#include <atomic>
#include <cstddef>

#include "audio_handles.h"
#include "audio_limits.h"
#include "voice_controls.h"

struct PlayDesc {
    float volume = 1.0f;
    float pitch = 1.0f;
};

class VoiceController : public IVoiceControls {
public:
    [[nodiscard]]
    VoiceParams params(size_t idx) const override;
    void mark_finished(size_t idx) override;

public:
    VoiceHandle claim(const PlayDesc &desc);
    void force_release(VoiceHandle handle);
    void stop_voice(VoiceHandle handle);
    void stop_voices_of_source(Handle source_handle);

    [[nodiscard]]
    bool is_playing(VoiceHandle voice_handle) const;

private:
    struct Slot {
        std::atomic<bool> playing{false};
        std::atomic<bool> stop_request{false};
        std::atomic<float> volume{0.0f};
        std::atomic<float> pitch{0.0f};
        std::atomic<size_t> generation{0};

        std::atomic<Handle::StorageType> source_index{0};
    };

    [[nodiscard]] Slot *get_slot_from_handle(Handle handle);
    [[nodiscard]] const Slot *get_slot_from_handle(VoiceHandle handle) const {
        return const_cast<VoiceController *>(this)->get_slot_from_handle(handle);
    }

    std::array<Slot, AudioLimits::MAX_VOICES> m_slots;
};
