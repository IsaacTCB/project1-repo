#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "sdl3_platform.h"

SDL_AppResult SDL_AppInit(void **appstate, [[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
    static PlatformImpl s_impl;
    *appstate = &s_impl;
    return s_impl.init();
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    auto *impl = static_cast<PlatformImpl *>(appstate);
    return impl->event(*event);
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    auto *impl = static_cast<PlatformImpl *>(appstate);
    return impl->iterate();
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    auto *impl = static_cast<PlatformImpl *>(appstate);
    impl->close(result);
}
