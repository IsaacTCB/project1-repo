#include <SDL3/SDL.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_main.h>

#include <cstdint>

#include "sdl3_common.h"
#include "sdl3_gamepad.h"
#include "sdl3_main.h"

#include "core/assertion.h"
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

static Key::Scancode convert_scancode(SDL_Scancode scancode) {
    ASSERT(scancode >= 0 && scancode < Key::SCANCODE_COUNT);
    return static_cast<Key::Scancode>(scancode);
}

static Key::MouseButtons convert_mouse_button(uint8_t button) {
    ASSERT(button > 0 && button < Key::MOUSE_BUTTON_COUNT);
    return static_cast<Key::MouseButtons>(button);
}

SDL_AppResult PlatformImpl::init() {
    log_init();

    if (!SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        LOG_CRITICAL("Failed to initialize SDL3. Reason: {0}", SDL_GetError());
        m_state.running = false;
        return SDL_APP_FAILURE;
    }

    if (int result = SDL_AddGamepadMappingsFromFile("gamecontrollerdb.txt") >= 0) {
        LOG_TRACE("Loaded {0} gamepad mappings.", result);
    } else if (result < 0) {
        LOG_ERROR("Failed to load gamecontrollerdb.txt mappings. Reason: {0}", SDL_GetError());
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

SDL_AppResult PlatformImpl::event(SDL_Event &event) {
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

        /* Keyboard/mouse input events */
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            m_platform.submit_event<EventInput>(EventInputKey{
                .key = convert_scancode(SDL_GetScancodeFromKey(event.key.key, &event.key.mod)),
                .scancode = convert_scancode(event.key.scancode),
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
                .button = convert_mouse_button(event.button.button),
                .pressed = event.button.down,
            });
            break;

        /* Gamepad input events */
        case SDL_EVENT_GAMEPAD_ADDED:
            if (auto *pad = gamepad_init(event.gdevice.which)) {
                m_platform.submit_event<EventInput>(EventInputGamepadConnected{
                    .pad = pad->index,
                    .name = pad->name,
                });
            } else {
                LOG_ERROR("Failed to add gamepad {0}", event.gdevice.which);
            }
            break;
        case SDL_EVENT_GAMEPAD_REMOVED:
            if (auto *pad = gamepad_find(event.gdevice.which)) {
                gamepad_close(event.gdevice.which);
                m_platform.submit_event<EventInput>(EventInputGamepadDisconnected{
                    .pad = pad->index,
                });
            } else {
                LOG_ERROR("Failed to close gamepad {0}", event.gdevice.which);
            }
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
            if (auto *pad = gamepad_find(event.gbutton.which)) {
                auto button = gamepad_sdl_to_button(static_cast<SDL_GamepadButton>(event.gbutton.button));
                if (button >= Gamepad::Buttons::COUNT) {
                    break;
                }
                ASSERT(event.gbutton.button == gamepad_button_to_sdl(button), "Mapping is incorrect");
                m_platform.submit_event<EventInput>(EventInputGamepadButton{
                    .pad = pad->index,
                    .button = button,
                    .pressed = event.gbutton.down,
                });
            }
            break;
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            if (auto *pad = gamepad_find(event.gaxis.which)) {
                auto axis = gamepad_sdl_to_axis(static_cast<SDL_GamepadAxis>(event.gaxis.axis));
                if (axis >= Gamepad::Axis::COUNT) {
                    break;
                }
                ASSERT(event.gaxis.axis == gamepad_axis_to_sdl(axis), "Mapping is incorrect");
                m_platform.submit_event<EventInput>(EventInputGamepadAxisMotion{
                    .pad = pad->index,
                    .axis = axis,
                    .value = static_cast<float>(event.gaxis.value) / 32768.0f,
                });
            }
            break;
    }
    return SDL_APP_CONTINUE;
}

void PlatformImpl::close(SDL_AppResult result) {
    m_platform.close();
    m_state.running = false;
}
