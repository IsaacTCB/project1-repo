#pragma once

struct PlatformImplState {
    bool running{false};

    bool has_open_display{false}; /* used by the display server */
};

void log_init();
