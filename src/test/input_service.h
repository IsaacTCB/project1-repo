#pragma once

#include "core/events/input_events.h"

#include "core/input/gamepad.h"
#include "core/input/keyboard.h"
#include "core/input/mouse.h"

#include <array>
#include <functional>
#include <tuple>
#include <type_traits>
#include <vector>

class InputService {
public:
    void init();
    void close();

    void update();
    void event(const EventInput &event);

    [[nodiscard]]
    bool is_key_pressed(Input::Key::Scancode scancode) const;

    [[nodiscard]]
    bool is_key_just_pressed(Input::Key::Scancode scancode) const;

    [[nodiscard]]
    bool is_key_just_released(Input::Key::Scancode scancode) const;

    [[nodiscard]]
    bool is_mouse_button_pressed(Input::Mouse::Buttons button) const;

    [[nodiscard]]
    bool is_mouse_button_just_pressed(Input::Mouse::Buttons button) const;

    [[nodiscard]]
    bool is_mouse_button_just_released(Input::Mouse::Buttons button) const;

    [[nodiscard]]
    std::tuple<float, float> get_mouse_pos() const {
        return {m_mouse_x, m_mouse_y};
    }

    [[nodiscard]]
    bool is_pad_button_pressed(Input::Gamepad::DeviceID device, Input::Gamepad::Buttons button) const;

    [[nodiscard]]
    bool is_pad_button_just_pressed(Input::Gamepad::DeviceID device, Input::Gamepad::Buttons button) const;

    [[nodiscard]]
    bool is_pad_button_just_released(Input::Gamepad::DeviceID device, Input::Gamepad::Buttons button) const;

    [[nodiscard]]
    std::tuple<float, float> get_pad_stick(Input::Gamepad::DeviceID device, bool right) const;

    [[nodiscard]]
    bool is_pad_connected(Input::Gamepad::DeviceID device) const {
        return m_pad_connected[device];
    }

    template <typename Func>
    void pad_foreach(Func &&func) {
        for (size_t i = 0; i < m_pad_connected.size(); i++) {
            if (!m_pad_connected[i]) {
                continue;
            }

            using ReturnType = std::invoke_result_t<Func, Input::Gamepad::DeviceID>;
            if constexpr (std::is_void_v<ReturnType>) {
                auto device = static_cast<Input::Gamepad::DeviceID>(i);
                std::invoke(func, device);
            } else {
                auto device = static_cast<Input::Gamepad::DeviceID>(i);
                if (std::invoke(func, device)) {
                    return;
                }
            }
        }
    }

private:
    bool keyboard_event(const EventInput &event);
    bool mouse_event(const EventInput &event);
    bool pad_event(const EventInput &event);

private:
    uint8_t m_mouse_state, m_mouse_state_previous;
    std::vector<bool> m_key_state;
    float m_mouse_x = 0.0f, m_mouse_y = 0.0f;

    struct PadState {
        uint32_t buttons = 0;
        std::array<float, Input::Gamepad::AXIS_COUNT> axis = {0};
    };

    std::array<bool, Input::Gamepad::GAMEPAD_MAX> m_pad_connected{};
    std::array<PadState, Input::Gamepad::GAMEPAD_MAX> m_pad_state{};
    std::array<PadState, Input::Gamepad::GAMEPAD_MAX> m_pad_state_previous{};
};
