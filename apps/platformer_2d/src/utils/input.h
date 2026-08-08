#pragma once

#include <array>
#include <cstddef>

#include <core/input/gamepad.h>
#include <core/input/keyboard.h>
#include <core/math/vec2.h>
#include <runtime/input_service.h>

struct InputActionBind {
    std::array<Input::Key::Scancode, 2> keys{};
    Input::Gamepad::Buttons buttons;
};

template <typename ActionEnum>
class InputActionSystem {
private:
    struct ActionState {
        bool held;
        bool just_pressed;
        bool just_released;
    };

private:
    [[nodiscard]]
    const InputActionBind *get_bind(ActionEnum action) const {
        auto idx = static_cast<size_t>(action);
        return idx < SIZE ? &m_actions[idx]
                          : nullptr;
    }

    [[nodiscard]]
    InputActionBind *get_bind(ActionEnum action) {
        const auto *const_this = const_cast<const InputActionSystem *>(this);
        return const_cast<InputActionBind *>(const_this->get_bind(action));
    }

    [[nodiscard]]
    const ActionState *get_state(ActionEnum action) const {
        auto idx = static_cast<size_t>(action);
        return idx < SIZE ? &m_actions_state[idx]
                          : nullptr;
    }

    [[nodiscard]]
    ActionState *get_state(ActionEnum action) {
        const auto *const_this = const_cast<const InputActionSystem *>(this);
        return const_cast<ActionState *>(const_this->get_state(action));
    }

public:
    void bind(ActionEnum action, InputActionBind new_bind) {
        auto *bind = get_bind(action);
        *bind = new_bind;
    }

    void update(const InputService &service) {
        for (auto &bind : m_actions) {
            size_t idx = &bind - m_actions.data();
            bool held = false;

            /* check bounded keys */
            for (auto scancode : bind.keys) {
                if (scancode == Input::Key::Scancode::UNKNOWN &&
                    static_cast<size_t>(scancode) >= Input::Key::SCANCODE_COUNT) {
                    continue;
                }

                if (service.is_key_pressed(scancode)) {
                    held = true;
                    break;
                }
            }

            /* check bounded gamepad button */
            if (bind.buttons != Input::Gamepad::Buttons::UNKNOWN) {
                service.pad_foreach([&](Input::Gamepad::DeviceID device) {
                    if (service.is_pad_button_pressed(device, bind.buttons)) {
                        held = true;
                        return true;
                    }
                    return false;
                });
            }

            /* update state */
            ActionState &state = m_actions_state[idx];
            state.just_pressed = held && !state.held;
            state.just_released = !held && state.held;
            state.held = held;
        }
    }

public:
    [[nodiscard]]
    bool is_action_pressed(ActionEnum action) const {
        const auto *bind = get_state(action);
        return bind ? bind->held : false;
    }

    [[nodiscard]]
    bool is_action_just_pressed(ActionEnum action) const {
        const auto *bind = get_state(action);
        return bind ? bind->just_pressed : false;
    }

    [[nodiscard]]
    bool is_action_just_released(ActionEnum action) const {
        const auto *bind = get_state(action);
        return bind ? bind->just_released : false;
    }

    [[nodiscard]]
    float get_axis(ActionEnum negative, ActionEnum positive) const {
        return (float)is_action_pressed(positive) - (float)is_action_pressed(negative);
    }

    Vec2 get_vector(ActionEnum negative_x, ActionEnum positive_x, ActionEnum negative_y, ActionEnum positive_y) {
        Vec2 vector{
            get_axis(negative_x, positive_x),
            get_axis(negative_y, positive_y),
        };
        vector.limit_length(1.0f);
        return vector;
    }

private:
    constexpr static auto SIZE = static_cast<size_t>(ActionEnum::COUNT);
    std::array<InputActionBind, SIZE> m_actions{};
    std::array<ActionState, SIZE> m_actions_state{};
};
