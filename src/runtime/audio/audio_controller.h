#pragma once

#include "core/utils/handles.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

#include "audio_handles.h"
#include "audio_limits.h"
#include "audio_source.h"
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
    void force_release_all();
    void force_release(VoiceHandle handle);
    IAudioSourceInstance *attach(VoiceHandle handle, std::unique_ptr<IAudioSourceInstance> instance);

    void stop_voice(VoiceHandle handle);
    void stop_voices_of_source(Handle source_handle);

    [[nodiscard]]
    bool is_playing(VoiceHandle voice_handle) const;

    void set_volume_gain(VoiceHandle voice_handle, float volume);

    void update();

private:
    enum class VoiceState : uint8_t {
        FREE,     /* slot is unused */
        PLAYING,  /* slot is being used for playback */
        FINISHED, /* playback has finished and main thread is yet to acknowledge it */
    };

    struct VoiceSlot {
        /* instance & instance state */
        std::unique_ptr<IAudioSourceInstance> instance;
        std::atomic<VoiceState> state{VoiceState::FREE};

        /* controls */
        std::atomic<bool> stop_request{false};
        std::atomic<float> volume{0.0f};
        std::atomic<float> pitch{0.0f};
        std::atomic<size_t> generation{0};

        /* audio source handle, used by stop_voices_of_source() */
        std::atomic<Handle::StorageType> source_handle{0};
    };

    [[nodiscard]] VoiceSlot *get_slot_from_handle(Handle handle);
    [[nodiscard]] const VoiceSlot *get_slot_from_handle(VoiceHandle handle) const {
        return const_cast<VoiceController *>(this)->get_slot_from_handle(handle);
    }

    std::array<VoiceSlot, AudioLimits::MAX_VOICES> m_slots;
};
