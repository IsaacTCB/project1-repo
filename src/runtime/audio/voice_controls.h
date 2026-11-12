#pragma once

#include <cstddef>

struct VoiceParams {
    bool stop_requested; /* true if voice controller demands this voice to be stopped. */

    float volume;
    float pitch;
    bool looping;
};

class IVoiceControls {
public:
    /**
     * @brief Returns a struct containing each
     * parameter value at the given voice index.
     *
     * This function is called by the audio thread.
     *
     * @param idx A valid voice index (typically VoiceHandle::index())
     * @return A VoiceParams object.
     */
    [[nodiscard]]
    virtual VoiceParams params(size_t idx) const = 0;

    /**
     * @brief Flags the voice at voice index as finished,
     * so that the audio controller can clean it up from
     * the main thread.
     *
     * This function is called by the audio thread.
     *
     * @param idx A valid voice index (typically VoiceHandle::index())
     */
    virtual void mark_finished(size_t idx) = 0;
};
