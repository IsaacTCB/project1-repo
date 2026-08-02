#pragma once

#include <optional>

#include "platform/api/event_server.h"
#include "runtime/runtime.h"

#include "sdl3_display_server.h"
#include "sdl3_io_server.h"
#include "sdl3_os_server.h"

constexpr uint32_t TARGET_FPS = 60;

class SDL3_PlatformImpl {
public:
    SDL_AppResult init(int argc, char *argv[]);
    SDL_AppResult iterate();
    SDL_AppResult event(SDL_Event &event);
    void close(SDL_AppResult result);

private:
    static bool apply_framelimit(uint64_t work_ms_time, uint64_t target_ms_rate);

private:
    std::optional<EngineRuntime> m_runtime;
    uint64_t m_last_tick{};

private:
    SDL3_DisplayServer m_display_server;
    SDL3_IOServer m_io_server;
    SDL3_OSServer m_os_server;
    EventServer m_event_server;
};
