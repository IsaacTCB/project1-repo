#pragma once

#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_joystick.h>

#include <string>

#include "core/keys/gamepad.h"

struct ImplGamepad {
    Gamepad::DeviceID index = 0;
    SDL_JoystickID which = 0;
    SDL_Gamepad *gamepad = nullptr;
    std::string name;
    bool alive = false;
};

/**
 * @brief Converts SDL_GamepadButton into Gamepad::Buttons.
 *
 * @param button SDL_GamepadButton for conversion
 *
 * @returns The corresponding Gamepad::Buttons, or Gamepad::Buttons::COUNT on failure.
 */
Gamepad::Buttons gamepad_sdl_to_button(SDL_GamepadButton button);

/**
 * @brief Converts Gamepad::Buttons into SDL_GamepadButton.
 *
 * @param button Gamepad::Buttons value for conversion
 *
 * @returns The corresponding SDL_GamepadButton, or SDL_GAMEPAD_BUTTON_INVALID on failure.
 */
SDL_GamepadButton gamepad_button_to_sdl(Gamepad::Buttons button);

/**
 * @brief Converts SDL_GamepadAxis into Gamepad::Axis.
 *
 * @param button SDL_GamepadAxis for conversion
 *
 * @returns The corresponding Gamepad::Axis, or Gamepad::Axis::COUNT on failure.
 */
Gamepad::Axis gamepad_sdl_to_axis(SDL_GamepadAxis axis);

/**
 * @brief Converts Gamepad::Axis into SDL_GamepadAxis.
 *
 * @param button Gamepad::Axis value for conversion
 *
 * @returns The corresponding SDL_GamepadAxis, or SDL_GAMEPAD_AXIS_INVALID on failure.
 */
SDL_GamepadAxis gamepad_axis_to_sdl(Gamepad::Axis axis);

ImplGamepad *gamepad_init(SDL_JoystickID which);
ImplGamepad *gamepad_find(SDL_JoystickID which);
bool gamepad_close(SDL_JoystickID which);
