#include "input_service.h"

#include "core/assertion.h"

#include "core/keys/gamepad.h"
#include "core/keys/scancodes.h"
#include "core/logger.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>

#define GET_BIT(index) (1 << index)

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

    std::ranges::copy(
        m_pad_state,
        m_pad_state_previous.begin());

    /* update previous key state */
    ASSERT(m_key_state.size() >= Key::SCANCODE_COUNT * 2,
        "Invalid m_key_state size!");
    std::copy(
        m_key_state.begin(),
        m_key_state.begin() + Key::SCANCODE_COUNT,
        m_key_state.begin() + Key::SCANCODE_COUNT);

    /* clear all disconnected pads. */
    for (size_t i = 0; i < Gamepad::GAMEPAD_MAX; i++) {
        if (!m_pad_connected[i]) {
            m_pad_state[i] = 0u;
            /* cleaning the previous state arent needed,
             * because i'd like for them to trigger on
             * pad disconnect. */
        }
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

[[nodiscard]]
static size_t get_pad_button_index(Gamepad::Buttons button) {
    auto index = static_cast<size_t>(button);
    return index;
}

[[nodiscard]]
static inline bool get_flag_at(uint8_t bitfield, size_t index) {
    uint8_t mask = GET_BIT(index);
    return (bitfield & mask) != 0;
}

void InputService::event(const EventInput &event) {
    /* Keyboard-related events */
    if (const auto *kbkey = std::get_if<EventInputKey>(&event)) {
        if (kbkey->repeat) {
            return;
        }
        size_t idx = get_scancode_index(kbkey->scancode);
        ASSERT(idx < Key::SCANCODE_COUNT, "Keyboard key out-of-range");
        m_key_state.at(idx) = kbkey->pressed;
        return;
    }
    /* Mouse-related events */
    if (const auto *mouse = std::get_if<EventInputMouseMotion>(&event)) {
        m_mouse_x = mouse->screen_x;
        m_mouse_y = mouse->screen_y;
        return;
    }
    if (const auto *mbutton = std::get_if<EventInputMouseButton>(&event)) {
        size_t idx = get_mouse_button_index(mbutton->button);
        ASSERT(idx < Key::MOUSE_BUTTON_COUNT - 1, "Mouse button out-of-range");

        uint8_t mask = GET_BIT(idx);
        if (mbutton->pressed) {
            /* turn mouse bit on */
            m_mouse_state |= mask;
        } else {
            /* turn mouse bit off*/
            m_mouse_state &= ~mask;
        }
        return;
    }
    /* Gamepad-related events */
    if (const auto *gamepad = std::get_if<EventInputGamepadConnected>(&event)) {
        LOG_TRACE("Gamepad {0} connected ({1})", gamepad->pad, gamepad->name);
        auto pad = static_cast<size_t>(gamepad->pad);
        m_pad_connected[pad] = true;
        return;
    }
    if (const auto *gamepad = std::get_if<EventInputGamepadDisconnected>(&event)) {
        LOG_TRACE("Gamepad {0} disconnected", gamepad->pad);
        auto pad = static_cast<size_t>(gamepad->pad);
        m_pad_connected[pad] = false;
        return;
    }
    if (const auto *gamepad = std::get_if<EventInputGamepadButton>(&event)) {
        auto pad = static_cast<size_t>(gamepad->pad);
        if (!m_pad_connected[pad]) {
            return;
        }

        size_t idx = get_pad_button_index(gamepad->button);
        ASSERT(idx < Gamepad::BUTTON_COUNT, "Gamepad button out-of-range");

        uint32_t mask = GET_BIT(idx);
        if (gamepad->pressed) {
            /* turn pad bit on */
            m_pad_state[pad] |= mask;
        } else {
            /* turn pad bit off */
            m_pad_state[pad] &= ~mask;
        }
        return;
    }
}

/* ============== */
/*    Keyboard    */
/* ============== */

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

/* ================= */
/*   Mouse buttons   */
/* ================= */

bool InputService::is_mouse_button_pressed(Key::MouseButtons button) const {
    size_t index = get_mouse_button_index(button);
    ASSERT(index < Key::MOUSE_BUTTON_COUNT);
    return get_flag_at(m_mouse_state, index);
}

bool InputService::is_mouse_button_just_pressed(Key::MouseButtons button) const {
    size_t index = get_mouse_button_index(button);
    ASSERT(index < Key::MOUSE_BUTTON_COUNT);
    return (get_flag_at(m_mouse_state, index) &&
            !get_flag_at(m_mouse_state_previous, index));
}

bool InputService::is_mouse_button_just_released(Key::MouseButtons button) const {
    size_t index = get_mouse_button_index(button);
    ASSERT(index < Key::MOUSE_BUTTON_COUNT);
    return (!get_flag_at(m_mouse_state, index) &&
            get_flag_at(m_mouse_state_previous, index));
}

/* =================== */
/*   Gamepad buttons   */
/* =================== */

bool InputService::is_pad_button_pressed(Gamepad::DeviceID device, Gamepad::Buttons button) const {
    auto pad = static_cast<size_t>(device);
    ASSERT(pad < Gamepad::GAMEPAD_MAX);

    size_t index = get_pad_button_index(button);
    ASSERT(index < Gamepad::BUTTON_COUNT);
    return get_flag_at(m_pad_state[pad], index);
}

bool InputService::is_pad_button_just_pressed(Gamepad::DeviceID device, Gamepad::Buttons button) const {
    auto pad = static_cast<size_t>(device);
    ASSERT(pad < Gamepad::GAMEPAD_MAX);

    size_t index = get_pad_button_index(button);
    ASSERT(index < Gamepad::BUTTON_COUNT);
    return (get_flag_at(m_pad_state[pad], index) &&
            !get_flag_at(m_pad_state_previous[pad], index));
}

bool InputService::is_pad_button_just_released(Gamepad::DeviceID device, Gamepad::Buttons button) const {
    auto pad = static_cast<size_t>(device);
    ASSERT(pad < Gamepad::GAMEPAD_MAX);

    size_t index = get_pad_button_index(button);
    ASSERT(index < Gamepad::BUTTON_COUNT);
    return (!get_flag_at(m_pad_state[pad], index) &&
            get_flag_at(m_pad_state_previous[pad], index));
}
