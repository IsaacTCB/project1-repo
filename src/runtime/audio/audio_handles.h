#pragma once

#include <bit>
#include <cstddef>

#include "core/utils/handles.h"

#include "audio_limits.h"

/* tags */
struct AudioSourceTag;
struct VoiceTag;

/* Handle aliases */
using AudioSourceHandle = Handle<AudioSourceTag>;

/* VoiceHandle is special because we only need
 * (AudioLimits::MAX_SLOTS + 1) indices (0 is reserved
 * for invalid handles). Most voice indices will eventually
 * get recycled, so generation capacity is more important
 * than ever. We use some C++ compile-time trickery to
 * calculate the maximum supported generation bit width
 * for the default StorageType (uint32_t) based off the
 * maximum amount of voice indices. */
template <>
struct HandleMeta<VoiceTag> {
    using StorageType = HandleDefaults::StorageType;
    static constexpr size_t IndexBits = std::bit_width(AudioLimits::MAX_SLOTS + 1);
    static constexpr size_t GenerationBits = std::numeric_limits<StorageType>::digits - IndexBits;
};
using VoiceHandle = Handle<VoiceTag>;
static_assert(VoiceHandle::INDEX_MAX >= AudioLimits::MAX_SLOTS - 1);
