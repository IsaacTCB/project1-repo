#pragma once

#include <cstddef>

namespace AudioLimits {

inline constexpr size_t OUTPUT_SAMPLE_RATE = 48000;
inline constexpr size_t OUTPUT_CHANNELS = 2;

inline constexpr size_t COMMAND_QUEUE_CAPACITY = 64;

inline constexpr size_t MAX_VOICES = 32;

} // namespace AudioLimits
