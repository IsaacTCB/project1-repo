#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_main.h>

#include "sdl3_common.h"
#include "sdl3_main.h"

#include "core/events/input_events.h"
#include "core/events/window_events.h"

#include "core/logger.h"

static PlatformImpl impl;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
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
    m_platform.init();

    return SDL_APP_CONTINUE;
}

SDL_AppResult PlatformImpl::iterate() {
    constexpr uint64_t TARGET_TICKRATE = (1.0 / TARGET_FPS) * 1000;

    uint64_t tick = SDL_GetTicks();

    m_platform.update(1.0f);
    m_platform.render();

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

        /* Window events */
        case SDL_EVENT_WINDOW_RESIZED:
            m_platform.submit_event<EventWindow>(EventWindowResized{
                .width = static_cast<uint32_t>(event.window.data1),
                .height = static_cast<uint32_t>(event.window.data2),
            });
            break;
        case SDL_EVENT_WINDOW_MOVED:
            m_platform.submit_event<EventWindow>(EventWindowMoved{
                .x = event.window.data1,
                .y = event.window.data2,
            });
            break;
        case SDL_EVENT_WINDOW_MAXIMIZED:
            m_platform.submit_event<EventWindow>(EventWindowMaximized{});
            break;
        case SDL_EVENT_WINDOW_MINIMIZED:
            m_platform.submit_event<EventWindow>(EventWindowMinimized{});
            break;

        /* Input events */
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            m_platform.submit_event<EventInput>(EventInputKey{
                .key = event.key.key,
                .scancode = event.key.scancode,
                .pressed = event.key.down,
                .repeat = event.key.repeat,
            });
            break;
        case SDL_EVENT_MOUSE_MOTION:
            m_platform.submit_event<EventInput>(EventInputMouseMotion{
                .screen_x = event.motion.x,
                .screen_y = event.motion.y,
                .rel_x = event.motion.xrel,
                .rel_y = event.motion.yrel,
            });
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            m_platform.submit_event<EventInput>(EventInputMouseButton{
                .button = event.button.button,
                .pressed = event.button.down,
            });
            break;
    }
    return SDL_APP_CONTINUE;
}

void PlatformImpl::close(SDL_AppResult result) {
    m_platform.close();
    m_state.running = false;
}
