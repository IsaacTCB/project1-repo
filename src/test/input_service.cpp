#include "input_service.h"

#include "core/assertion.h"

#include "core/keys/scancodes.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

#define GET_MOUSE_BIT(index) (1 << index)

void InputService::init() {
    m_mouse_state = 0;
    m_mouse_state_previous = 0;

    /* SoA containing both current pressed keys and last pressed keys
    This gets stored contigously in memory, like: [keys][last keys] */
    m_key_state.resize(Key::SCANCODE_COUNT * 2, false);
}

void InputService::close() {
    m_key_state.clear();
}

void InputService::update() {
    m_mouse_state_previous = m_mouse_state;
    for (size_t i = 0; i < Key::SCANCODE_COUNT; ++i) {
        m_key_state[Key::SCANCODE_COUNT + i] = m_key_state[i];
    }
}

[[nodiscard]]
static size_t get_scancode_index(Key::Scancode scancode) {
    return static_cast<size_t>(scancode);
}

[[nodiscard]]
static size_t get_mouse_button_index(Key::MouseButtons button) {
    auto index = static_cast<size_t>(button);
    ASSERT(index > 0);
    return index - 1;
}

void InputService::event(const EventInput &event) {
    if (const auto *ptr = std::get_if<EventInputKey>(&event)) {
        if (ptr->repeat) {
            return;
        }
        size_t idx = get_scancode_index(ptr->scancode);
        m_key_state.at(idx) = ptr->pressed;
    }
    if (const auto *ptr = std::get_if<EventInputMouseMotion>(&event)) {
        m_mouse_x = ptr->screen_x;
        m_mouse_y = ptr->screen_y;
    }
    if (const auto *ptr = std::get_if<EventInputMouseButton>(&event)) {
        size_t idx = get_mouse_button_index(ptr->button);
        uint8_t mask = GET_MOUSE_BIT(idx);
        if (ptr->pressed) {
            m_mouse_state |= mask;
        } else {
            m_mouse_state &= ~mask;
        }
    }
}

bool InputService::is_key_pressed(Key::Scancode scancode) const {
    size_t index = get_scancode_index(scancode);
    ASSERT(index < Key::SCANCODE_COUNT);
    return m_key_state[index];
}

bool InputService::is_key_just_pressed(Key::Scancode scancode) const {
    size_t index = get_scancode_index(scancode);
    ASSERT(index < Key::SCANCODE_COUNT);
    return m_key_state[index] && !m_key_state[Key::SCANCODE_COUNT + index];
}

bool InputService::is_key_just_released(Key::Scancode scancode) const {
    size_t index = get_scancode_index(scancode);
    ASSERT(index < Key::SCANCODE_COUNT);
    return !m_key_state.at(index) && m_key_state.at(Key::SCANCODE_COUNT + index);
}

static inline bool get_mouse_state_flag_at(uint8_t bitfield, size_t index) {
    uint8_t mask = GET_MOUSE_BIT(index);
    return (bitfield & mask) != 0;
}

bool InputService::is_mouse_button_pressed(Key::MouseButtons button) const {
    size_t index = get_mouse_button_index(button);
    ASSERT(index < Key::MOUSE_BUTTON_COUNT);
    return get_mouse_state_flag_at(m_mouse_state, index);
}

bool InputService::is_mouse_button_just_pressed(Key::MouseButtons button) const {
    size_t index = get_mouse_button_index(button);
    ASSERT(index < Key::MOUSE_BUTTON_COUNT);
    return (get_mouse_state_flag_at(m_mouse_state, index) &&
            !get_mouse_state_flag_at(m_mouse_state_previous, index));
}

bool InputService::is_mouse_button_just_released(Key::MouseButtons button) const {
    size_t index = get_mouse_button_index(button);
    ASSERT(index < Key::MOUSE_BUTTON_COUNT);
    return (!get_mouse_state_flag_at(m_mouse_state, index) &&
            get_mouse_state_flag_at(m_mouse_state_previous, index));
}
