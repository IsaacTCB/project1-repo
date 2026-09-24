#include "audio_controller.h"

VoiceHandle VoiceController::claim(const PlayDesc &desc) {
    for (size_t i = 0; i < m_slots.size(); ++i) {
        auto &slot = m_slots[i];

        bool expected = false;
        if (slot.playing.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
            slot.stop_request.store(false, std::memory_order_relaxed);

            slot.volume.store(desc.volume, std::memory_order_relaxed);
            slot.pitch.store(desc.pitch, std::memory_order_relaxed);

            const size_t gen = slot.generation.load(std::memory_order_relaxed);
            return Handle::from_index_gen(i, gen);
        }
    }
    return Handle{};
}

void VoiceController::force_release(VoiceHandle handle) {
    if (auto *slot = get_slot_from_handle(handle)) {
        slot->playing.store(false, std::memory_order_release);
    }
}

void VoiceController::stop_voice(VoiceHandle handle) {
    if (auto *slot = get_slot_from_handle(handle)) {
        slot->stop_request.store(true, std::memory_order_release);
    }
}

void VoiceController::stop_voices_of_source(Handle source_handle) {
    auto source_index = source_handle.index();
    for (auto &slot : m_slots) {
        if (slot.playing.load(std::memory_order_acquire) &&
            slot.source_index == source_index) {
            slot.stop_request.store(true, std::memory_order_release);
        }
    }
}

VoiceParams VoiceController::params(size_t idx) const {
    const auto &slot = m_slots[idx];
    return {
        .volume = slot.volume.load(std::memory_order_acquire),
        .pitch = slot.pitch.load(std::memory_order_acquire),
        .stop_requested = slot.stop_request.load(std::memory_order_acquire),
    };
}

void VoiceController::mark_finished(size_t idx) {
    auto &slot = m_slots[idx];
    slot.generation.fetch_add(1, std::memory_order_relaxed);
    slot.playing.store(false, std::memory_order_release);
}

VoiceController::Slot *VoiceController::get_slot_from_handle(Handle handle) {
    if (!handle || handle.index() >= m_slots.size()) {
        return nullptr;
    }

    auto &slot = m_slots[handle.index()];
    if (handle.gen() != slot.generation.load(std::memory_order_relaxed)) {
        return nullptr;
    }

    return &slot;
}

bool VoiceController::is_playing(VoiceHandle voice_handle) const {
    if (const auto *slot = get_slot_from_handle(voice_handle)) {
        return (slot->playing.load(std::memory_order_acquire) &&
                !slot->stop_request.load(std::memory_order_relaxed));
    }
    return false;
}
