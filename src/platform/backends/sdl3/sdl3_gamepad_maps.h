#pragma once

#include "core/keys/gamepad.h"
#include <SDL3/SDL_gamepad.h>

/**
 * @brief X-macro conversion table between SDL_GamepadButton and ::Gamepad::Buttons.
 *
 * @param X X-macro implementation. Must contain 2 parameters, which are
 * the SDL enum type, followed by the corresponding engine button.
 */
#define GAMEPAD_BUTTON_MAP(X)                                              \
    X(SDL_GAMEPAD_BUTTON_SOUTH, Gamepad::Buttons::SOUTH)                   \
    X(SDL_GAMEPAD_BUTTON_EAST, Gamepad::Buttons::EAST)                     \
    X(SDL_GAMEPAD_BUTTON_WEST, Gamepad::Buttons::WEST)                     \
    X(SDL_GAMEPAD_BUTTON_NORTH, Gamepad::Buttons::NORTH)                   \
    X(SDL_GAMEPAD_BUTTON_BACK, Gamepad::Buttons::BACK)                     \
    X(SDL_GAMEPAD_BUTTON_GUIDE, Gamepad::Buttons::GUIDE)                   \
    X(SDL_GAMEPAD_BUTTON_START, Gamepad::Buttons::START)                   \
    X(SDL_GAMEPAD_BUTTON_LEFT_STICK, Gamepad::Buttons::LEFT_STICK)         \
    X(SDL_GAMEPAD_BUTTON_RIGHT_STICK, Gamepad::Buttons::RIGHT_STICK)       \
    X(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, Gamepad::Buttons::LEFT_SHOULDER)   \
    X(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, Gamepad::Buttons::RIGHT_SHOULDER) \
    X(SDL_GAMEPAD_BUTTON_DPAD_UP, Gamepad::Buttons::DPAD_UP)               \
    X(SDL_GAMEPAD_BUTTON_DPAD_DOWN, Gamepad::Buttons::DPAD_DOWN)           \
    X(SDL_GAMEPAD_BUTTON_DPAD_LEFT, Gamepad::Buttons::DPAD_LEFT)           \
    X(SDL_GAMEPAD_BUTTON_DPAD_RIGHT, Gamepad::Buttons::DPAD_RIGHT)         \
    X(SDL_GAMEPAD_BUTTON_MISC1, Gamepad::Buttons::MISC1)                   \
    X(SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1, Gamepad::Buttons::RIGHT_PADDLE1)   \
    X(SDL_GAMEPAD_BUTTON_LEFT_PADDLE1, Gamepad::Buttons::LEFT_PADDLE1)     \
    X(SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2, Gamepad::Buttons::RIGHT_PADDLE2)   \
    X(SDL_GAMEPAD_BUTTON_LEFT_PADDLE2, Gamepad::Buttons::LEFT_PADDLE2)     \
    X(SDL_GAMEPAD_BUTTON_TOUCHPAD, Gamepad::Buttons::TOUCHPAD)             \
    X(SDL_GAMEPAD_BUTTON_MISC2, Gamepad::Buttons::MISC2)                   \
    X(SDL_GAMEPAD_BUTTON_MISC3, Gamepad::Buttons::MISC3)                   \
    X(SDL_GAMEPAD_BUTTON_MISC4, Gamepad::Buttons::MISC4)                   \
    X(SDL_GAMEPAD_BUTTON_MISC5, Gamepad::Buttons::MISC5)                   \
    X(SDL_GAMEPAD_BUTTON_MISC6, Gamepad::Buttons::MISC6)

/**
 * @brief X-macro conversion table between SDL_GamepadAxis and ::Gamepad::Axis.
 *
 * @param X X-macro implementation. Must contain 2 parameters, which are
 * the SDL enum type, followed by the corresponding engine axis.
 */
#define GAMEPAD_AXIS_MAP(X)                                       \
    X(SDL_GAMEPAD_AXIS_LEFTX, Gamepad::Axis::LEFT_HORIZONTAL)     \
    X(SDL_GAMEPAD_AXIS_LEFTY, Gamepad::Axis::LEFT_VERTICAL)       \
    X(SDL_GAMEPAD_AXIS_RIGHTX, Gamepad::Axis::RIGHT_HORIZONTAL)   \
    X(SDL_GAMEPAD_AXIS_RIGHTY, Gamepad::Axis::RIGHT_VERTICAL)     \
    X(SDL_GAMEPAD_AXIS_LEFT_TRIGGER, Gamepad::Axis::TRIGGER_LEFT) \
    X(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, Gamepad::Axis::TRIGGER_RIGHT)
