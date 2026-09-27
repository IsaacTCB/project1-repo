#pragma once

#include <cstddef>

struct VoiceParams {
    float volume;
    float pitch;
    bool stop_requested;

    bool looping;
};

class IVoiceControls {
public:
    [[nodiscard]]
    virtual VoiceParams params(size_t idx) const = 0;
    virtual void mark_finished(size_t idx) = 0;
};
