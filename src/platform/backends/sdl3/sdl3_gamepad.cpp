#include "sdl3_gamepad.h"
#include "sdl3_gamepad_maps.h"

#include <array>
#include <stack>

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

template <typename Enum, typename Table>
static constexpr auto gamepad__enum_lookup(
    Enum value, const Table &table, typename Table::value_type fallback)
    -> typename Table::value_type {
    using Type = std::conditional_t<
        std::is_enum_v<Enum>,
        std::underlying_type_t<Enum>,
        Enum>;

    const Type idx = static_cast<Type>(value);
    if (idx < 0 || static_cast<size_t>(idx) > table.size()) {
        return fallback;
    }
    return table[static_cast<size_t>(idx)];
}

Gamepad::Buttons gamepad_sdl_to_button(SDL_GamepadButton button) {
    static constexpr auto s_sdl_to_btn_lookup = build_sdl_to_btn_lookup();
    return gamepad__enum_lookup(button, s_sdl_to_btn_lookup, Gamepad::Buttons::COUNT);
}

SDL_GamepadButton gamepad_button_to_sdl(Gamepad::Buttons button) {
    static constexpr auto s_btn_to_sdl_lookup = build_btn_to_sdl_lookup();
    return gamepad__enum_lookup(button, s_btn_to_sdl_lookup, SDL_GAMEPAD_BUTTON_INVALID);
}

Gamepad::Axis gamepad_sdl_to_axis(SDL_GamepadAxis axis) {
    static constexpr auto s_sdl_to_axis_lookup = build_sdl_to_axis_lookup();
    return gamepad__enum_lookup(axis, s_sdl_to_axis_lookup, Gamepad::Axis::COUNT);
}

SDL_GamepadAxis gamepad_axis_to_sdl(Gamepad::Axis axis) {
    static constexpr auto s_axis_to_sdl_lookup = build_axis_to_sdl_lookup();
    return gamepad__enum_lookup(axis, s_axis_to_sdl_lookup, SDL_GAMEPAD_AXIS_INVALID);
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
