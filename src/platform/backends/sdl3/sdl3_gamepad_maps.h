#pragma once

#include "core/input/gamepad.h"
#include <SDL3/SDL_gamepad.h>

#include <array>

/**
 * @brief X-macro conversion table between SDL_GamepadButton and ::Input::Gamepad::Buttons.
 *
 * @param X X-macro implementation. Must contain 2 parameters, which are
 * the SDL enum type, followed by the corresponding engine button.
 */
#define GAMEPAD_BUTTON_MAP(X)                                                     \
    X(SDL_GAMEPAD_BUTTON_SOUTH, Input::Gamepad::Buttons::SOUTH)                   \
    X(SDL_GAMEPAD_BUTTON_EAST, Input::Gamepad::Buttons::EAST)                     \
    X(SDL_GAMEPAD_BUTTON_WEST, Input::Gamepad::Buttons::WEST)                     \
    X(SDL_GAMEPAD_BUTTON_NORTH, Input::Gamepad::Buttons::NORTH)                   \
    X(SDL_GAMEPAD_BUTTON_BACK, Input::Gamepad::Buttons::BACK)                     \
    X(SDL_GAMEPAD_BUTTON_GUIDE, Input::Gamepad::Buttons::GUIDE)                   \
    X(SDL_GAMEPAD_BUTTON_START, Input::Gamepad::Buttons::START)                   \
    X(SDL_GAMEPAD_BUTTON_LEFT_STICK, Input::Gamepad::Buttons::LEFT_STICK)         \
    X(SDL_GAMEPAD_BUTTON_RIGHT_STICK, Input::Gamepad::Buttons::RIGHT_STICK)       \
    X(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, Input::Gamepad::Buttons::LEFT_SHOULDER)   \
    X(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, Input::Gamepad::Buttons::RIGHT_SHOULDER) \
    X(SDL_GAMEPAD_BUTTON_DPAD_UP, Input::Gamepad::Buttons::DPAD_UP)               \
    X(SDL_GAMEPAD_BUTTON_DPAD_DOWN, Input::Gamepad::Buttons::DPAD_DOWN)           \
    X(SDL_GAMEPAD_BUTTON_DPAD_LEFT, Input::Gamepad::Buttons::DPAD_LEFT)           \
    X(SDL_GAMEPAD_BUTTON_DPAD_RIGHT, Input::Gamepad::Buttons::DPAD_RIGHT)         \
    X(SDL_GAMEPAD_BUTTON_MISC1, Input::Gamepad::Buttons::MISC1)                   \
    X(SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1, Input::Gamepad::Buttons::RIGHT_PADDLE1)   \
    X(SDL_GAMEPAD_BUTTON_LEFT_PADDLE1, Input::Gamepad::Buttons::LEFT_PADDLE1)     \
    X(SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2, Input::Gamepad::Buttons::RIGHT_PADDLE2)   \
    X(SDL_GAMEPAD_BUTTON_LEFT_PADDLE2, Input::Gamepad::Buttons::LEFT_PADDLE2)     \
    X(SDL_GAMEPAD_BUTTON_TOUCHPAD, Input::Gamepad::Buttons::TOUCHPAD)             \
    X(SDL_GAMEPAD_BUTTON_MISC2, Input::Gamepad::Buttons::MISC2)                   \
    X(SDL_GAMEPAD_BUTTON_MISC3, Input::Gamepad::Buttons::MISC3)                   \
    X(SDL_GAMEPAD_BUTTON_MISC4, Input::Gamepad::Buttons::MISC4)                   \
    X(SDL_GAMEPAD_BUTTON_MISC5, Input::Gamepad::Buttons::MISC5)                   \
    X(SDL_GAMEPAD_BUTTON_MISC6, Input::Gamepad::Buttons::MISC6)

/**
 * @brief X-macro conversion table between SDL_GamepadAxis and ::Input::Gamepad::Axis.
 *
 * @param X X-macro implementation. Must contain 2 parameters, which are
 * the SDL enum type, followed by the corresponding engine axis.
 */
#define GAMEPAD_AXIS_MAP(X)                                              \
    X(SDL_GAMEPAD_AXIS_LEFTX, Input::Gamepad::Axis::LEFT_HORIZONTAL)     \
    X(SDL_GAMEPAD_AXIS_LEFTY, Input::Gamepad::Axis::LEFT_VERTICAL)       \
    X(SDL_GAMEPAD_AXIS_RIGHTX, Input::Gamepad::Axis::RIGHT_HORIZONTAL)   \
    X(SDL_GAMEPAD_AXIS_RIGHTY, Input::Gamepad::Axis::RIGHT_VERTICAL)     \
    X(SDL_GAMEPAD_AXIS_LEFT_TRIGGER, Input::Gamepad::Axis::TRIGGER_LEFT) \
    X(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, Input::Gamepad::Axis::TRIGGER_RIGHT)

/**
 * @brief Builds SDL button -> engine button lookup table
 *
 * @return a std::array lookup table.
 */
static consteval auto build_sdl_to_btn_lookup() -> decltype(auto) {
    std::array<Input::Gamepad::Buttons, SDL_GAMEPAD_BUTTON_COUNT> lookup{};
    lookup.fill(Input::Gamepad::Buttons::COUNT);
#define MAP(sdl3_btn, btn) lookup[sdl3_btn] = btn;
    GAMEPAD_BUTTON_MAP(MAP);
#undef MAP
    return lookup;
}

/**
 * @brief Builds engine button -> SDL button lookup table
 *
 * @return a std::array lookup table.
 */
static consteval auto build_btn_to_sdl_lookup() -> decltype(auto) {
    std::array<SDL_GamepadButton, Input::Gamepad::BUTTON_COUNT> lookup{};
    lookup.fill(SDL_GAMEPAD_BUTTON_INVALID);
#define MAP(sdl3_btn, btn) lookup[static_cast<size_t>(btn)] = sdl3_btn;
    GAMEPAD_BUTTON_MAP(MAP);
#undef MAP
    return lookup;
}

/**
 * @brief Builds SDL axis -> engine axis lookup table
 *
 * @return a std::array lookup table.
 */
static consteval auto build_sdl_to_axis_lookup() -> decltype(auto) {
    std::array<Input::Gamepad::Axis, SDL_GAMEPAD_AXIS_COUNT> lookup{};
    lookup.fill(Input::Gamepad::Axis::COUNT);
#define MAP(sdl3_axis, axis) lookup[sdl3_axis] = axis;
    GAMEPAD_AXIS_MAP(MAP);
#undef MAP
    return lookup;
}

/**
 * @brief Builds engine axis -> SDL axis lookup table
 *
 * @return a std::array lookup table.
 */
static consteval auto build_axis_to_sdl_lookup() -> decltype(auto) {
    std::array<SDL_GamepadAxis, Input::Gamepad::AXIS_COUNT> lookup{};
    lookup.fill(SDL_GAMEPAD_AXIS_INVALID);
#define MAP(sdl3_axis, axis) lookup[static_cast<size_t>(axis)] = sdl3_axis;
    GAMEPAD_AXIS_MAP(MAP);
#undef MAP
    return lookup;
}
