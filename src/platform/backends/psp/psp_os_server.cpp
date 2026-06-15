#include "platform/api/os_server.h"

#include "psp_common.h"

#include <psprtc.h>

void OSServer::exit_app() {
    m_state.running = false;
}

const char *OSServer::get_os() const {
    return "PlayStation Portable";
}

const char *OSServer::get_platform() const {
    return "psp";
}

float OSServer::get_ticks_sec() const {
    return 0.0f;
    // return (float)SDL_GetTicks() / 1000.0f;
}

uint64_t OSServer::get_ticks_ms() const {
    return 0;
    // return SDL_GetTicks();
}
