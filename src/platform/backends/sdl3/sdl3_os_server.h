#pragma once

#include "runtime/server/os_server.h"

#include <SDL3/SDL.h>

class SDL3_OSServer final : public IOSServer {
public:
    void exit_app() override {
        m_running = false;
    }

    [[nodiscard]]
    const char *get_os() const override {
        return SDL_GetPlatform();
    }

    [[nodiscard]]
    const char *get_platform() const override {
        return "sdl3";
    }

    [[nodiscard]]
    uint64_t get_ticks_ms() const override {
        return SDL_GetTicks();
    }

public:
    [[nodiscard]]
    bool is_running() const {
        return m_running;
    }

private:
    bool m_running = true;
};
