#include "../os_server.h"

#include "sdl3_common.h"

const char* OSServer::get_os() const {
    return SDL_GetPlatform();
}

const char* OSServer::get_platform() const {
    return "sdl3";
}