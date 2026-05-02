#include "sdl3_gamepad.h"

#include <array>
#include <stack>

#include <SDL3/SDL_joystick.h>

#include "core/keys/gamepad.h"
#include "core/logger.h"

static std::array<ImplGamepad, Gamepad::GAMEPAD_MAX> s_gamepads;
static std::stack<size_t> s_gamepads_available;

static ImplGamepad *gamepad__reserve_slot() {
    ImplGamepad *slot = nullptr;

    if (!s_gamepads_available.empty()) {
        /* get slot from stack */
        size_t idx = s_gamepads_available.top();
        slot = &s_gamepads[idx];
        s_gamepads_available.pop();
    } else {
        /* find new allocated slot */
        size_t index = 0;
        for (auto &gamepad : s_gamepads) {
            gamepad.index = index++;
            if (!gamepad.alive) {
                slot = &gamepad;
                break;
            }
        }
    }

    return slot;
}

ImplGamepad *gamepad_init(SDL_JoystickID which) {
    auto *gamepad = gamepad__reserve_slot();
    if (!gamepad) {
        return nullptr;
    }

    SDL_Gamepad *raw_gamepad = SDL_OpenGamepad(which);
    if (!raw_gamepad) {
        LOG_ERROR("Gamepad {0} added, but not opened: {1}", (unsigned int)which, SDL_GetError());
        gamepad->alive = false;
        return nullptr;
    }

    std::string name = SDL_GetGamepadName(raw_gamepad);
    LOG_TRACE("Gamepad {0} ({1}) added", which, name);

    *gamepad = {
        .index = gamepad->index,
        .which = which,
        .gamepad = raw_gamepad,
        .name = std::move(name),
        .alive = true,
    };
    return gamepad;
}

ImplGamepad *gamepad_find(SDL_JoystickID which) {
    for (auto &gamepad : s_gamepads) {
        if (gamepad.alive && gamepad.which == which) {
            return &gamepad;
        }
    }
    return nullptr;
}

bool gamepad_close(SDL_JoystickID which) {
    auto *gamepad = gamepad_find(which);
    if (!gamepad) {
        return false;
    }

    SDL_CloseGamepad(gamepad->gamepad);
    gamepad->alive = false;

    auto idx = static_cast<size_t>(gamepad - s_gamepads.data());
    s_gamepads_available.push(idx);
    return true;
}
