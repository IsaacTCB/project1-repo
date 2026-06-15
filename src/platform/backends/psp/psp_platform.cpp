#include "psp_platform.h"

#include <cstdint>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <pspctrl.h>
#pragma GCC diagnostic pop

#include <pspuser.h>

#include "core/events/input_events.h"
#include "core/events/platform_events.h"
#include "core/events/window_events.h"
#include "core/input/gamepad.h"

#include "platform/backends/psp/psp_common.h"

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

    m_platform.update(60.0f / 1.0f);
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

    std::uint32_t diff = pad.Buttons ^ last_pad.Buttons;

    /* TODO: refactor this whole system. X-macros would be great
     * (view sdl3_gamepad_maps.h to see what I mean) */

    /* dpad buttons */
    update_pad_btn(Input::Gamepad::Buttons::DPAD_UP, PSP_CTRL_UP, diff, pad.Buttons);
    update_pad_btn(Input::Gamepad::Buttons::DPAD_RIGHT, PSP_CTRL_RIGHT, diff, pad.Buttons);
    update_pad_btn(Input::Gamepad::Buttons::DPAD_DOWN, PSP_CTRL_DOWN, diff, pad.Buttons);
    update_pad_btn(Input::Gamepad::Buttons::DPAD_LEFT, PSP_CTRL_LEFT, diff, pad.Buttons);

    /* face buttons */
    update_pad_btn(Input::Gamepad::Buttons::NORTH, PSP_CTRL_TRIANGLE, diff, pad.Buttons);
    update_pad_btn(Input::Gamepad::Buttons::EAST, PSP_CTRL_CIRCLE, diff, pad.Buttons);
    update_pad_btn(Input::Gamepad::Buttons::SOUTH, PSP_CTRL_CROSS, diff, pad.Buttons);
    update_pad_btn(Input::Gamepad::Buttons::WEST, PSP_CTRL_SQUARE, diff, pad.Buttons);

    /* shoulder buttons */
    update_pad_btn(Input::Gamepad::Buttons::LEFT_SHOULDER, PSP_CTRL_L1, diff, pad.Buttons);
    update_pad_btn(Input::Gamepad::Buttons::RIGHT_SHOULDER, PSP_CTRL_R1, diff, pad.Buttons);

    /* row buttons */
    update_pad_btn(Input::Gamepad::Buttons::MISC1, PSP_CTRL_NOTE, diff, pad.Buttons);
    update_pad_btn(Input::Gamepad::Buttons::MISC2, PSP_CTRL_SCREEN, diff, pad.Buttons);
    update_pad_btn(Input::Gamepad::Buttons::MISC5, PSP_CTRL_VOLDOWN, diff, pad.Buttons);
    update_pad_btn(Input::Gamepad::Buttons::MISC6, PSP_CTRL_VOLUP, diff, pad.Buttons);

#if defined(PSP_ENABLE_CONTROLLER_EXTENSION)
    /* enables extra buttons from the Dualshock 3 */
    update_pad_btn(Input::Gamepad::Buttons::LEFT_STICK, PSP_CTRL_L3, diff, pad.Buttons);
    update_pad_btn(Input::Gamepad::Buttons::RIGHT_STICK, PSP_CTRL_R3, diff, pad.Buttons);
#endif

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
#endif /* PSP_ENABLE_CONTROLLER_EXTENSION */

    last_pad = pad;
}

void PlatformImpl::update_pad_btn(Input::Gamepad::Buttons button, uint32_t mask, uint32_t diff, uint32_t current) {
    if (diff & mask) {
        m_platform.event().submit<EventInput>(EventInputGamepadButton{
            .pad = DEFAULT_PAD,
            .button = button,
            .pressed = (current & mask) != 0,
        });
    }
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
