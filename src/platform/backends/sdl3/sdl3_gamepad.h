#pragma once

#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_joystick.h>

#include <string>

struct ImplGamepad {
    size_t index = 0;
    SDL_JoystickID which = 0;
    SDL_Gamepad *gamepad = nullptr;
    std::string name;
    bool alive = false;
};

ImplGamepad *gamepad_init(SDL_JoystickID which);
ImplGamepad *gamepad_find(SDL_JoystickID which);
bool gamepad_close(SDL_JoystickID which);
