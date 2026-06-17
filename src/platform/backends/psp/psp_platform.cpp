#include "psp_platform.h"

#include <cstdint>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <pspctrl.h>
#include <pspuser.h>
#pragma GCC diagnostic pop

#include "core/events/input_events.h"
#include "core/events/platform_events.h"
#include "platform/backends/psp/psp_common.h"
#include "platform/backends/psp/psp_gamepad_maps.h"

/* Define PSP modules */
PSP_MODULE_INFO("engine", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_VFPU | THREAD_ATTR_USER);

/* The PSP, as far as I'm aware of, cannot have multiple controllers
 * connected at once. You can pair a DS3 into a PSPGo, but I don't
 * think it's possible to use it as a separate gamepad natively.
 * To simplify stuff, let's just hardcode everything into the same
 * gamepad device id. */
constexpr Input::Gamepad::DeviceID DEFAULT_PAD = 0;
constexpr std::string_view DEFAULT_PAD_NAME = "PSP Controller";

static bool s_exited_from_callback = false;

static void setup_callbacks();

PlatformAppState PlatformImpl::init([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
    setup_callbacks();
    log_init();

    /* enable pad & analog */
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    m_state.running = true;
    m_platform.init();

    /* notify gamepad connection */
    m_platform.submit_event<EventInput>(EventInputGamepadConnected{
        .pad = DEFAULT_PAD,
        .name = DEFAULT_PAD_NAME,
    });

    return PlatformAppState::CONTINUE;
}

PlatformAppState PlatformImpl::iterate() {
    poll_inputs();

    /* let the engine handle the exit request */
    if (s_exited_from_callback) {
        m_platform.submit_event<EventPlatformExited>();
        s_exited_from_callback = false;
    }

    m_platform.update(1.0f / 60.0f);
    m_platform.render();

    if (!m_state.running) {
        return PlatformAppState::QUIT;
    }

    return PlatformAppState::CONTINUE;
}

void PlatformImpl::poll_inputs() {
    static SceCtrlData last_pad{};

    SceCtrlData pad{};
    sceCtrlPeekBufferPositive(&pad, 1);

    uint32_t diff = pad.Buttons ^ last_pad.Buttons;

#define MAP(mask, btn)                                                 \
    if (diff & mask) {                                                 \
        m_platform.event().submit<EventInput>(EventInputGamepadButton{ \
            .pad = DEFAULT_PAD,                                        \
            .button = btn,                                             \
            .pressed = (pad.Buttons & mask) != 0,                      \
        });                                                            \
    }
    GAMEPAD_BUTTON_MAP(MAP)
#if defined(PSP_ENABLE_CONTROLLER_EXTENSION)
    GAMEPAD_EXTENDED_BUTTON_MAP(MAP)
#endif
#undef MAP

    /* update gamepad analog stick */
    if (pad.Lx != last_pad.Lx) {
        m_platform.event().submit<EventInput>(EventInputGamepadAxisMotion{
            .pad = DEFAULT_PAD,
            .axis = Input::Gamepad::Axis::LEFT_HORIZONTAL,
            .value = static_cast<float>((int)(pad.Lx) - 128) / 128.0f,
        });
    }
    if (pad.Ly != last_pad.Ly) {
        m_platform.event().submit<EventInput>(EventInputGamepadAxisMotion{
            .pad = DEFAULT_PAD,
            .axis = Input::Gamepad::Axis::LEFT_VERTICAL,
            .value = static_cast<float>((int)(pad.Ly) - 128) / 128.0f,
        });
    }

#if defined(PSP_ENABLE_CONTROLLER_EXTENSION)
    /* enables the right analog stick from the PSVita & Dualshock 3 */
    if (pad.Rx != last_pad.Rx) {
        m_platform.event().submit<EventInput>(EventInputGamepadAxisMotion{
            .pad = DEFAULT_PAD,
            .axis = Input::Gamepad::Axis::RIGHT_HORIZONTAL,
            .value = static_cast<float>((int)(pad.Rx) - 128) / 128.0f,
        });
    }
    if (pad.Ry != last_pad.Ry) {
        m_platform.event().submit<EventInput>(EventInputGamepadAxisMotion{
            .pad = DEFAULT_PAD,
            .axis = Input::Gamepad::Axis::RIGHT_VERTICAL,
            .value = static_cast<float>((int)(pad.Ry) - 128) / 128.0f,
        });
    }
    /* simulate analog triggers */
    if (diff & PSP_CTRL_L2) {
        m_platform.event().submit<EventInput>(EventInputGamepadAxisMotion{
            .pad = DEFAULT_PAD,
            .axis = Input::Gamepad::Axis::TRIGGER_LEFT,
            .value = (pad.Buttons & PSP_CTRL_L2) ? 1.0f : 0.0f,
        });
    }
    if (diff & PSP_CTRL_R2) {
        m_platform.event().submit<EventInput>(EventInputGamepadAxisMotion{
            .pad = DEFAULT_PAD,
            .axis = Input::Gamepad::Axis::TRIGGER_RIGHT,
            .value = (pad.Buttons & PSP_CTRL_R2) ? 1.0f : 0.0f,
        });
    }
#endif /* PSP_ENABLE_CONTROLLER_EXTENSION */

    last_pad = pad;
}

void PlatformImpl::close([[maybe_unused]] PlatformAppState app_state) {
    m_platform.close();
    m_state.running = false;
    sceKernelExitGame();
}

int PlatformImpl::exit_callback([[maybe_unused]] int arg1, [[maybe_unused]] int arg2, [[maybe_unused]] void *common) {
    s_exited_from_callback = true;
    return 0;
}

/*************/

int callback_thread([[maybe_unused]] SceSize args, [[maybe_unused]] void *argp) {
    int cbid = sceKernelCreateCallback("Exit Callback", PlatformImpl::exit_callback, nullptr);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

void setup_callbacks() {
    int thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, nullptr);
    if (thid >= 0) {
        sceKernelStartThread(thid, 0, nullptr);
    }
}
