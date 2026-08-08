#pragma once

#include "core/events/input_events.h"
#include "core/math/vec2.h"

#include "core/input/gamepad.h"
#include "core/input/keyboard.h"
#include "core/input/mouse.h"

#include <array>
#include <functional>
#include <limits>
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
    Vec2 get_mouse_pos() const { return m_mouse_position; }

    [[nodiscard]]
    float get_mouse_wheel_x() const { return m_mouse_wheel.x; }

    [[nodiscard]]
    float get_mouse_wheel_y() const { return m_mouse_wheel.y; }

    [[nodiscard]]
    bool is_pad_button_pressed(Input::Gamepad::DeviceID device, Input::Gamepad::Buttons button) const;

    [[nodiscard]]
    bool is_pad_button_just_pressed(Input::Gamepad::DeviceID device, Input::Gamepad::Buttons button) const;

    [[nodiscard]]
    bool is_pad_button_just_released(Input::Gamepad::DeviceID device, Input::Gamepad::Buttons button) const;

    [[nodiscard]]
    Vec2 get_pad_stick(Input::Gamepad::DeviceID device, bool right) const;

    [[nodiscard]]
    bool is_pad_connected(Input::Gamepad::DeviceID device) const {
        return m_pad_connected[device];
    }

    /**
     * @brief Executes a callback for every active gamepad.
     *
     * @tparam Func A callable type accepting Input::Gamepad::DeviceID.
     * @param func Callback function. If it returns true, then iteration ends early.
     *             Can return void for unconditional iteration.
     */
    template <typename Func>
        requires std::is_invocable_r_v<bool, Func, Input::Gamepad::DeviceID> ||
                 std::is_invocable_r_v<void, Func, Input::Gamepad::DeviceID>
    void pad_foreach(Func &&func) const;

private:
    bool keyboard_event(const EventInput &event);
    bool mouse_event(const EventInput &event);
    bool pad_event(const EventInput &event);

private:
    uint8_t m_mouse_state, m_mouse_state_previous;
    std::vector<bool> m_key_state;
    Vec2 m_mouse_position;
    Vec2 m_mouse_wheel;

    struct PadState {
        uint32_t buttons = 0;
        std::array<float, Input::Gamepad::AXIS_COUNT> axis = {0};
    };

    std::array<bool, Input::Gamepad::GAMEPAD_MAX> m_pad_connected{};
    std::array<PadState, Input::Gamepad::GAMEPAD_MAX> m_pad_state{};
    std::array<PadState, Input::Gamepad::GAMEPAD_MAX> m_pad_state_previous{};

    /* safety check */
    static_assert(
        Input::Mouse::BUTTON_COUNT < std::numeric_limits<decltype(m_mouse_state)>::digits,
        "Mouse Button count is too big for InputService::m_mouse_state! Consider changing its underlying type.");

    static_assert(
        Input::Gamepad::BUTTON_COUNT < std::numeric_limits<decltype(PadState::buttons)>::digits,
        "Gamepad Button count is too big for InputService::PadState::buttons! Consider changing its underlying type.");
};

template <typename Func>
    requires std::is_invocable_r_v<bool, Func, Input::Gamepad::DeviceID> ||
             std::is_invocable_r_v<void, Func, Input::Gamepad::DeviceID>
void InputService::pad_foreach(Func &&func) const {
    for (size_t i = 0; i < m_pad_connected.size(); i++) {
        if (!m_pad_connected[i]) {
            continue;
        }

        auto device = static_cast<Input::Gamepad::DeviceID>(i);

        using ReturnType = std::invoke_result_t<Func, Input::Gamepad::DeviceID>;
        if constexpr (std::is_void_v<ReturnType>) {
            std::invoke(func, device);
        } else {
            if (std::invoke(func, device)) {
                return;
            }
        }
    }
}
