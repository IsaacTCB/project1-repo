#pragma once

#include "core/events/input_events.h"
#include "core/keys/scancodes.h"

#include <tuple>
#include <vector>

class InputService {
public:
    void init();
    void close();

    void update();
    void event(const EventInput &event);

    [[nodiscard]]
    bool is_key_pressed(Key::Scancode scancode) const;

    [[nodiscard]]
    bool is_key_just_pressed(Key::Scancode scancode) const;

    [[nodiscard]]
    bool is_key_just_released(Key::Scancode scancode) const;

    [[nodiscard]]
    bool is_mouse_button_pressed(Key::MouseButtons button) const;

    [[nodiscard]]
    bool is_mouse_button_just_pressed(Key::MouseButtons button) const;

    [[nodiscard]]
    bool is_mouse_button_just_released(Key::MouseButtons button) const;

    [[nodiscard]]
    std::tuple<float, float> get_mouse_pos() const {
        return {m_mouse_x, m_mouse_y};
    }

private:
    uint8_t m_mouse_state, m_mouse_state_previous;
    std::vector<bool> m_key_state;
    float m_mouse_x = 0.0f, m_mouse_y = 0.0f;
};
