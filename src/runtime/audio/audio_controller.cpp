#include "audio_controller.h"

#include <atomic>

#include "audio_handles.h"
#include "audio_source.h"
#include "core/assertion.h"

VoiceHandle VoiceController::claim(const PlayDesc &desc, AudioSourceHandle source_handle) {
    for (size_t i = 0; i < m_slots.size(); ++i) {
        auto &slot = m_slots[i];

        VoiceState expected = VoiceState::FREE;
        if (slot.state.compare_exchange_strong(expected, VoiceState::PLAYING, std::memory_order_acq_rel)) {
            slot.stop_request.store(false, std::memory_order_relaxed);

            slot.volume.store(desc.volume, std::memory_order_relaxed);
            slot.pitch.store(desc.pitch, std::memory_order_relaxed);
            slot.looping.store(desc.looping, std::memory_order_relaxed);

            slot.source_handle.store(source_handle.value(), std::memory_order_relaxed);

            const size_t gen = slot.generation.load(std::memory_order_relaxed);
            return VoiceHandle::from_index_gen(i, gen);
        }
    }
    return VoiceHandle{};
}

IAudioSourceInstance *VoiceController::attach(VoiceHandle handle, std::unique_ptr<IAudioSourceInstance> instance) {
    auto *slot = get_slot_from_handle(handle);
    ASSERT(slot, "Attempted to attach into a non-existent handle");
    slot->instance = std::move(instance);
    return slot->instance.get();
}

void VoiceController::force_release(VoiceHandle handle) {
    if (auto *slot = get_slot_from_handle(handle)) {
        slot->state.store(VoiceState::FREE, std::memory_order_release);
        slot->generation.fetch_add(1, std::memory_order_relaxed);
        slot->instance.reset();
    }
}

void VoiceController::force_release_all() {
    for (auto &slot : m_slots) {
        slot.state.store(VoiceState::FREE, std::memory_order_release);
        slot.generation.fetch_add(1, std::memory_order_relaxed);
        slot.instance.reset();
    }
}

void VoiceController::stop_voice(VoiceHandle handle) {
    if (auto *slot = get_slot_from_handle(handle)) {
        slot->stop_request.store(true, std::memory_order_release);
    }
}

void VoiceController::stop_voices_of_source(AudioSourceHandle source_handle) {
    for (auto &slot : m_slots) {
        if (slot.state.load(std::memory_order_acquire) == VoiceState::PLAYING &&
            slot.source_handle.load(std::memory_order_acquire) == source_handle.value()) {
            slot.stop_request.store(true, std::memory_order_release);
        }
    }
}

VoiceParams VoiceController::params(size_t idx) const {
    const auto &slot = m_slots[idx];
    return {
        .stop_requested = slot.stop_request.load(std::memory_order_acquire),
        .volume = slot.volume.load(std::memory_order_acquire),
        .pitch = slot.pitch.load(std::memory_order_acquire),
        .looping = slot.looping.load(std::memory_order_acquire),
    };
}

void VoiceController::mark_finished(size_t idx) {
    auto &slot = m_slots[idx];
    slot.state.store(VoiceState::FINISHED, std::memory_order_release);
}

void VoiceController::update() {
    for (auto &slot : m_slots) {
        if (slot.state.load(std::memory_order_acquire) != VoiceState::FINISHED) {
            continue;
        }

        slot.instance.reset();
        slot.generation.fetch_add(1, std::memory_order_relaxed);
        slot.state.store(VoiceState::FREE, std::memory_order_release);
    }
}

VoiceController::VoiceSlot *VoiceController::get_slot_from_handle(VoiceHandle handle) {
    if (!handle || handle.index() >= m_slots.size()) {
        return nullptr;
    }

    auto &slot = m_slots[handle.index()];
    if (handle.gen() != slot.generation.load(std::memory_order_relaxed)) {
        return nullptr;
    }

    return &slot;
}

/* Voice Getters */

bool VoiceController::is_playing(VoiceHandle voice_handle) const {
    if (const auto *slot = get_slot_from_handle(voice_handle)) {
        return (slot->state.load(std::memory_order_acquire) == VoiceState::PLAYING &&
                !slot->stop_request.load(std::memory_order_relaxed));
    }
    return false;
}

bool VoiceController::is_looping(VoiceHandle voice_handle) const {
    if (const auto *slot = get_slot_from_handle(voice_handle)) {
        /* FIXME: this getter does not care about voice state */
        return slot->looping.load(std::memory_order_acquire);
    }
    return false;
}

/* Voice Setters */

void VoiceController::set_looping(VoiceHandle voice_handle, bool looping) {
    if (auto *slot = get_slot_from_handle(voice_handle)) {
        /* FIXME: this setter does not care about voice state */
        slot->looping.store(looping, std::memory_order_release);
    }
}

void VoiceController::set_volume_gain(VoiceHandle voice_handle, float volume) {
    if (auto *slot = get_slot_from_handle(voice_handle)) {
        /* FIXME: this setter does not care about voice state */
        slot->volume.store(volume, std::memory_order_release);
    }
}
