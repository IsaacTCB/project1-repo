#include "input_service.h"

#include "core/assertion.h"
#include "core/logger.h"

#include "core/keys/scancodes.h"

#include <cstring>

void InputService::init() {
    /* SoA containing both current pressed keys and last pressed keys
    This gets stored contigously in memory, like: [keys][last keys] */
    m_key_state.resize(Key::SCANCODE_COUNT * 2, false);
}

void InputService::close() {
    m_key_state.clear();
}

void InputService::update() {
    for (size_t i = 0; i < Key::SCANCODE_COUNT; ++i) {
        m_key_state[Key::SCANCODE_COUNT + i] = m_key_state[i];
    }
}

[[nodiscard]]
static size_t get_scancode_index(Key::Scancode scancode) {
    return static_cast<size_t>(scancode);
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
        LOG_TRACE("{} {}",
            ptr->pressed ? "Pressed" : "Released",
            ptr->button);
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
