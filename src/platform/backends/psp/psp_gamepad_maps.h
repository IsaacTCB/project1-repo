#pragma once

/**
 * @brief X-macro conversion table between PSP ctrl masks and ::Input::Gamepad::Buttons.
 *
 * @param X X-macro implementation. Must contain 2 parameters, which are
 * the PSP ctrl mask, followed by the corresponding engine button.
 */
#define GAMEPAD_BUTTON_MAP(X)                                     \
    X(PSP_CTRL_UP, Input::Gamepad::Buttons::DPAD_UP)              \
    X(PSP_CTRL_RIGHT, Input::Gamepad::Buttons::DPAD_RIGHT)        \
    X(PSP_CTRL_DOWN, Input::Gamepad::Buttons::DPAD_DOWN)          \
    X(PSP_CTRL_TRIANGLE, Input::Gamepad::Buttons::NORTH)          \
    X(PSP_CTRL_CIRCLE, Input::Gamepad::Buttons::EAST)             \
    X(PSP_CTRL_CROSS, Input::Gamepad::Buttons::SOUTH)             \
    X(PSP_CTRL_SQUARE, Input::Gamepad::Buttons::WEST)             \
    X(PSP_CTRL_LTRIGGER, Input::Gamepad::Buttons::LEFT_SHOULDER)  \
    X(PSP_CTRL_RTRIGGER, Input::Gamepad::Buttons::RIGHT_SHOULDER) \
    X(PSP_CTRL_NOTE, Input::Gamepad::Buttons::MISC1)              \
    X(PSP_CTRL_SCREEN, Input::Gamepad::Buttons::MISC2)            \
    X(PSP_CTRL_VOLDOWN, Input::Gamepad::Buttons::MISC5)           \
    X(PSP_CTRL_VOLUP, Input::Gamepad::Buttons::MISC6)

/**
 * @brief Same as ::GAMEPAD_BUTTON_MAP, but only for extended
 * buttons
 */
#define GAMEPAD_EXTENDED_BUTTON_MAP(X)                  \
    X(PSP_CTRL_L3, Input::Gamepad::Buttons::LEFT_STICK) \
    X(PSP_CTRL_R3, Input::Gamepad::Buttons::RIGHT_STICK)
