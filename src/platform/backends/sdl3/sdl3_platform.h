#pragma once

#include "platform/api/platform.h"

#include "sdl3_common.h"

constexpr uint32_t TARGET_FPS = 60;

class PlatformImpl {
public:
    PlatformImpl()
        : m_state{}, m_platform{m_state} {}

    SDL_AppResult init(int argc, char *argv[]);
    SDL_AppResult iterate();
    SDL_AppResult event(SDL_Event &event);
    void close(SDL_AppResult result);

    bool is_running() const noexcept { return m_state.running; }

private:
    static bool apply_framelimit(uint64_t work_ms_time, uint64_t target_ms_rate);

private:
    uint64_t m_last_tick{};

    PlatformImplState m_state;
    Platform m_platform;
};
