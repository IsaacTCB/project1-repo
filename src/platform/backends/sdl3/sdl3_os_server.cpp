#include "platform/api/os_server.h"

#include <SDL3/SDL.h>

const char* OSServer::get_os() const {
    return SDL_GetPlatform();
}

const char* OSServer::get_platform() const {
    return "sdl3";
}
