#pragma once

#include "platform/backends/sdl3/sdl3_common.h"
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

    void set_fps_max(uint32_t fps) override {
        if (fps != g_fps_max) {
            g_fps_max = fps;
            g_fps_tickrate = fps > 0 ? (1'000'000'000 / fps) : 0;
        }
    }

    [[nodiscard]]
    uint32_t get_fps_max() override {
        return g_fps_max;
    }

public:
    [[nodiscard]]
    bool is_running() const {
        return m_running;
    }

private:
    bool m_running = true;
};
