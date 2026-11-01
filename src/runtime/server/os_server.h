#pragma once

#include <cstdint>

#include "server_macros.inl"

class IOSServer {
    DECLARE_SERVER(IOSServer)

public:
    virtual void exit_app() = 0;

    [[nodiscard]]
    virtual const char *get_os() const = 0;

    [[nodiscard]]
    virtual const char *get_platform() const = 0;

    [[nodiscard]]
    virtual float get_ticks_sec() const {
        return static_cast<float>(get_ticks_ms()) / 1000.0f;
    }

    [[nodiscard]]
    virtual uint64_t get_ticks_ms() const = 0;

    /**
     * @brief Enables the fps limiter
     *
     * @param fps The maximum fps, or 0 to disable fps limiter.
     */
    virtual void set_fps_max(uint32_t fps) = 0;

    /**
     * @brief Gets the current fps cap value
     *
     * @return The maximum fps, or 0 if it's disabled.
     */
    [[nodiscard]]
    virtual uint32_t get_fps_max() = 0;
};
