#include "platform/api/os_server.h"

#include <SDL3/SDL.h>

const char *OSServer::get_os() const {
    return SDL_GetPlatform();
}

const char *OSServer::get_platform() const {
    return "sdl3";
}

float OSServer::get_ticks_sec() const {
    return SDL_GetTicks() / 1000.0f;
}

uint64_t OSServer::get_ticks_ms() const {
    return SDL_GetTicks();
}
