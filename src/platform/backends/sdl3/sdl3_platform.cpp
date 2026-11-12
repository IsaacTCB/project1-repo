#include "sdl3_platform.h"

#include <SDL3/SDL_init.h>
#include <cstdint>
#include <span>

#include "runtime/runtime.h"
#include "sdl3_common.h"
#include "sdl3_gamepad.h"

#include "core/assertion.h"
#include "core/events/input_events.h"
#include "core/events/platform_events.h"
#include "core/events/window_events.h"
#include "core/logger.h"
#include "runtime/app/application.h"
#include "runtime/platform_services.h"

#if ENABLE_IMGUI
#include <backends/imgui_impl_sdl3.h>
#include <imgui.h>
#endif

class ApplicationHandler final : public IApplicationHandler {
public:
    ApplicationHandler() = default;

public:
    IApplication *create_app(const IApplication::Servers &app_servers, const PlatformServices &platform_services) override {
        return IApplication::create_application(app_servers, platform_services);
    }
};

static ApplicationHandler s_app_handler;

static Input::Key::Scancode convert_scancode(SDL_Scancode scancode) {
    ASSERT(scancode >= 0 && scancode < Input::Key::SCANCODE_COUNT);
    return static_cast<Input::Key::Scancode>(scancode);
}

static Input::Mouse::Buttons convert_mouse_button(uint8_t button) {
    ASSERT(button > 0 && button < Input::Mouse::BUTTON_COUNT);
    return static_cast<Input::Mouse::Buttons>(button);
}

SDL_AppResult SDL3_PlatformImpl::init(int argc, char *argv[]) {
    log_init();

    m_runtime.emplace(PlatformServices{
        .display = m_display_server,
        .io = m_io_server,
        .os = m_os_server,
        .event = m_event_server,
    });

    std::span<char *const> args{};
    if (argv && argc > 0) {
        args = {argv, static_cast<size_t>(argc)};
    }

    switch (m_runtime->process_launch_args(args)) {
        case EngineRuntime::LaunchResult::CONTINUE:
            break;
        case EngineRuntime::LaunchResult::EXIT_OK:
            return SDL_APP_SUCCESS;
        case EngineRuntime::LaunchResult::EXIT_ERROR:
            return SDL_APP_FAILURE;
        default:
            ASSERT(false, "Unrecognized launch state");
            return SDL_APP_FAILURE;
    }

    if (!SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) {
        LOG_CRITICAL("Failed to initialize SDL3. Reason: {0}", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    m_last_tick = SDL_GetTicksNS();
    m_runtime->init(s_app_handler);

    if (int mappings = SDL_AddGamepadMappingsFromFile("gamecontrollerdb.txt"); mappings >= 0) {
        LOG_INFO("Loaded {0} gamepad {1} from 'gamecontrollerdb.txt'.", mappings, mappings == 1 ? "mapping" : "mappings");
    } else if (mappings == -1) {
        LOG_WARN("Failed to load 'gamecontrollerdb.txt' mappings. Reason: {0}", SDL_GetError());
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL3_PlatformImpl::iterate() {
    /* get delta */
    uint64_t current_tick = SDL_GetTicksNS();
    float delta = static_cast<float>(current_tick - m_last_tick) / 1000000000.0f;
    m_last_tick = current_tick;

    /* process game */
    m_runtime->update(delta);
    m_runtime->render();

    /* apply framelimit */
    if (g_fps_tickrate) {
        uint64_t work_time = SDL_GetTicksNS() - current_tick;
        apply_framelimit(work_time, g_fps_tickrate);
    }

    /* clear any existing events
     *WARNING : removing this WILL cause memory leaks */
    m_event_server.clear();

    /* exit or continue loop */
    if (!m_os_server.is_running()) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

#if ENABLE_IMGUI
/**
 * @brief Used to filter out input events from
 * propagating into the runtime layer while ImGui
 * is focused.
 *
 * This function explicitly ignores release events
 * so that runtime doesn't think a key is being
 * held forever.
 *
 * @param event Current SDL event being processed
 * @return Returns true if ImGui uses this event.
 */
static bool imgui_captures_event(const SDL_Event &event) {
    if (!ImGui::GetCurrentContext()) {
        return false;
    }

    const ImGuiIO &io = ImGui::GetIO();
    switch (event.type) {
        case SDL_EVENT_KEY_DOWN:
            return io.WantCaptureKeyboard;
        case SDL_EVENT_MOUSE_MOTION:
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_WHEEL:
            return io.WantCaptureMouse;
        default:
            return false;
    }
}
#endif

SDL_AppResult SDL3_PlatformImpl::event(SDL_Event &event) {
#if ENABLE_IMGUI
    if (ImGui::GetCurrentContext() && ImGui::GetIO().BackendPlatformUserData) {
        ImGui_ImplSDL3_ProcessEvent(&event);
    }

    if (imgui_captures_event(event)) {
        /* early exit, don't propagate event
         * into runtime */
        return SDL_APP_CONTINUE;
    }
#endif

    switch (event.type) {
        case SDL_EVENT_QUIT:
            m_event_server.submit<EventPlatformExited>();
            break;

        /* Window events */
        case SDL_EVENT_WINDOW_RESIZED:
            m_event_server.submit<EventWindow>(EventWindowResized{
                .size = {
                    event.window.data1,
                    event.window.data2,
                },
            });
            break;
        case SDL_EVENT_WINDOW_MOVED:
            m_event_server.submit<EventWindow>(EventWindowMoved{
                .position = {
                    event.window.data1,
                    event.window.data2,
                },
            });
            break;
        case SDL_EVENT_WINDOW_MAXIMIZED:
            m_event_server.submit<EventWindow>(EventWindowMaximized{});
            break;
        case SDL_EVENT_WINDOW_MINIMIZED:
            m_event_server.submit<EventWindow>(EventWindowMinimized{});
            break;

        /* Keyboard/mouse input events */
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            m_event_server.submit<EventInput>(EventInputKey{
                .key = convert_scancode(SDL_GetScancodeFromKey(event.key.key, &event.key.mod)),
                .scancode = convert_scancode(event.key.scancode),
                .pressed = event.key.down,
                .repeat = event.key.repeat,
            });
            break;
        case SDL_EVENT_MOUSE_MOTION:
            m_event_server.submit<EventInput>(EventInputMouseMotion{
                .screen_position = {
                    event.motion.x,
                    event.motion.y,
                },
                .relative = {
                    event.motion.xrel,
                    event.motion.yrel,
                },
            });
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            m_event_server.submit<EventInput>(EventInputMouseButton{
                .button = convert_mouse_button(event.button.button),
                .pressed = event.button.down,
            });
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            m_event_server.submit<EventInput>(EventInputMouseWheel{
                .wheel_motion = {
                    event.wheel.x,
                    event.wheel.y,
                },
                .flipped = (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED),
            });
            break;

        /* Gamepad input events */
        case SDL_EVENT_GAMEPAD_ADDED:
            if (auto *pad = gamepad_init(event.gdevice.which)) {
                m_event_server.submit<EventInput>(EventInputGamepadConnected{
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
                m_event_server.submit<EventInput>(EventInputGamepadDisconnected{
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
                if (button >= Input::Gamepad::Buttons::COUNT) {
                    break;
                }
                ASSERT(event.gbutton.button == gamepad_button_to_sdl(button), "Mapping is incorrect");
                m_event_server.submit<EventInput>(EventInputGamepadButton{
                    .pad = pad->index,
                    .button = button,
                    .pressed = event.gbutton.down,
                });
            }
            break;
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            if (auto *pad = gamepad_find(event.gaxis.which)) {
                auto axis = gamepad_sdl_to_axis(static_cast<SDL_GamepadAxis>(event.gaxis.axis));
                if (axis >= Input::Gamepad::Axis::COUNT) {
                    break;
                }
                ASSERT(event.gaxis.axis == gamepad_axis_to_sdl(axis), "Mapping is incorrect");
                m_event_server.submit<EventInput>(EventInputGamepadAxisMotion{
                    .pad = pad->index,
                    .axis = axis,
                    .value = static_cast<float>(event.gaxis.value) / 32768.0f,
                });
            }
            break;
    }
    return SDL_APP_CONTINUE;
}

void SDL3_PlatformImpl::close(SDL_AppResult result) {
    m_runtime->close();

    if (result != SDL_APP_SUCCESS) {
        LOG_ERROR("App closed with errors.");
    }

    m_os_server.exit_app();
}

bool SDL3_PlatformImpl::apply_framelimit(uint64_t work_ms_time, uint64_t target_ms_rate) {
    bool capped = false;

    if (work_ms_time < target_ms_rate) {
        SDL_DelayNS(target_ms_rate - work_ms_time);
        capped = true;
    }

    return capped;
}
