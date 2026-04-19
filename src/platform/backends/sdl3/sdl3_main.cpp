#include <SDL3/SDL_init.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>

#include "sdl3_common.h"
#include "sdl3_main.h"

#include "core/logger.h"
#include "test/test.h"

static PlatformImpl impl;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char* argv[]) {
    return impl.init();
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    return impl.event(*event);
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    return impl.iterate();
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    impl.close(result);
}

SDL_AppResult PlatformImpl::init() {
    log_init();

    if (!SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO)) {
        LOG_CRITICAL("Failed to initialize SDL3");
        m_state.running = false;
        return SDL_APP_FAILURE;
    }

    m_state.running = true;
    m_app_handler.init();

    return SDL_APP_CONTINUE;
}

SDL_AppResult PlatformImpl::iterate() {
    constexpr uint64_t TARGET_TICKRATE = (1.0/TARGET_FPS) * 1000;

    uint64_t tick = SDL_GetTicks();

    m_app_handler.update(1.0f);
    m_app_handler.render();

    if (!m_state.running) {
        return SDL_APP_SUCCESS;
    }

    uint64_t delta = SDL_GetTicks() - tick;
    if (delta < TARGET_TICKRATE) {
        SDL_Delay(TARGET_TICKRATE - delta);
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult PlatformImpl::event(const SDL_Event &event) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
        m_state.running = false;
        break;
    }
    return SDL_APP_CONTINUE;
}

void PlatformImpl::close(SDL_AppResult result) {
    m_app_handler.close();
    m_state.running = false;
}
