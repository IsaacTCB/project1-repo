#include "input_service.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include "core/assertion.h"
#include "core/debug/profiler/profiler.h"
#include "core/input/gamepad.h"
#include "core/input/keyboard.h"
#include "core/input/mouse.h"
#include "core/logger.h"

#define GET_BIT(index) (1u << index)

void InputService::init() {
    m_mouse_state = 0;
    m_mouse_state_previous = 0;

    /* SoA containing both current pressed keys and last pressed keys
    This gets stored contigously in memory, like: [keys][last keys] */
    m_key_state.resize(Input::Key::SCANCODE_COUNT * 2, false);
}

void InputService::close() {
    m_key_state.clear();
}

void InputService::update() {
    PROFILE_FUNCTION_SCOPE();

    m_mouse_state_previous = m_mouse_state;

    /* update previous pad state */
    std::ranges::copy(
        m_pad_state,
        m_pad_state_previous.begin());

    /* update previous key state */
    ASSERT(m_key_state.size() >= Input::Key::SCANCODE_COUNT * 2,
        "Invalid m_key_state size!");
    std::copy(
        m_key_state.begin(),
        m_key_state.begin() + Input::Key::SCANCODE_COUNT,
        m_key_state.begin() + Input::Key::SCANCODE_COUNT);

    /* clear all disconnected pads. */
    for (size_t i = 0; i < Input::Gamepad::GAMEPAD_MAX; i++) {
        if (!m_pad_connected[i]) {
            /* cleaning the previous state isnt needed,
             * because i'd like for them to trigger on
             * pad disconnect. */
            auto *state = &m_pad_state[i];
            state->buttons = 0u;
            state->axis.fill(0.0f);
        }
    }

    /* reset mouse wheel */
    m_mouse_wheel.clear();
}

void InputService::event(const EventInput &event) {
    static constexpr std::array event_handlers{
        &InputService::keyboard_event,
        &InputService::mouse_event,
        &InputService::pad_event,
    };

    for (const auto ev : event_handlers) {
        bool handled = (this->*ev)(event);
        if (handled) {
            return;
        }
    }

    ASSERT(false, "Unsupported event!");
}

template <typename T>
[[nodiscard]]
static inline bool get_flag_at(T bitfield, size_t index) {
    T mask = static_cast<T>(GET_BIT(index));
    return (bitfield & mask) != 0;
}

/* ============== */
/*    Keyboard    */
/* ============== */

[[nodiscard]]
static size_t get_scancode_index(Input::Key::Scancode scancode) {
    return static_cast<size_t>(scancode);
}

bool InputService::keyboard_event(const EventInput &event) {
    if (const auto *kbkey = std::get_if<EventInputKey>(&event)) {
        if (kbkey->repeat) {
            return true;
        }
        size_t idx = get_scancode_index(kbkey->scancode);
        ASSERT(idx < Input::Key::SCANCODE_COUNT, "Keyboard key out-of-range");
        m_key_state.at(idx) = kbkey->pressed;
        return true;
    }

    return false;
}

bool InputService::is_key_pressed(Input::Key::Scancode scancode) const {
    size_t index = get_scancode_index(scancode);
    ASSERT(index < Input::Key::SCANCODE_COUNT);
    return m_key_state[index];
}

bool InputService::is_key_just_pressed(Input::Key::Scancode scancode) const {
    size_t index = get_scancode_index(scancode);
    ASSERT(index < Input::Key::SCANCODE_COUNT);
    return m_key_state[index] && !m_key_state[Input::Key::SCANCODE_COUNT + index];
}

bool InputService::is_key_just_released(Input::Key::Scancode scancode) const {
    size_t index = get_scancode_index(scancode);
    ASSERT(index < Input::Key::SCANCODE_COUNT);
    return !m_key_state.at(index) && m_key_state.at(Input::Key::SCANCODE_COUNT + index);
}

/* ================= */
/*   Mouse buttons   */
/* ================= */

[[nodiscard]]
static size_t get_mouse_button_index(Input::Mouse::Buttons button) {
    auto index = static_cast<size_t>(button);
    ASSERT(index > 0);
    return index - 1;
}

bool InputService::mouse_event(const EventInput &event) {
    if (const auto *mouse = std::get_if<EventInputMouseMotion>(&event)) {
        m_mouse_position = mouse->screen_position;
        return true;
    }
    if (const auto *mbutton = std::get_if<EventInputMouseButton>(&event)) {
        size_t idx = get_mouse_button_index(mbutton->button);
        ASSERT(idx < Input::Mouse::BUTTON_COUNT - 1, "Mouse button out-of-range");

        auto mask = static_cast<uint8_t>(GET_BIT(idx));
        if (mbutton->pressed) {
            /* turn mouse bit on */
            m_mouse_state |= mask;
        } else {
            /* turn mouse bit off*/
            m_mouse_state &= ~mask;
        }
        return true;
    }
    if (const auto *wheel = std::get_if<EventInputMouseWheel>(&event)) {
        m_mouse_wheel += wheel->wheel_motion;
        return true;
    }
    return false;
}

bool InputService::is_mouse_button_pressed(Input::Mouse::Buttons button) const {
    size_t index = get_mouse_button_index(button);
    ASSERT(index < Input::Mouse::BUTTON_COUNT);
    return get_flag_at(m_mouse_state, index);
}

bool InputService::is_mouse_button_just_pressed(Input::Mouse::Buttons button) const {
    size_t index = get_mouse_button_index(button);
    ASSERT(index < Input::Mouse::BUTTON_COUNT);
    return (get_flag_at(m_mouse_state, index) &&
            !get_flag_at(m_mouse_state_previous, index));
}

bool InputService::is_mouse_button_just_released(Input::Mouse::Buttons button) const {
    size_t index = get_mouse_button_index(button);
    ASSERT(index < Input::Mouse::BUTTON_COUNT);
    return (!get_flag_at(m_mouse_state, index) &&
            get_flag_at(m_mouse_state_previous, index));
}

/* =========== */
/*   Gamepad   */
/* =========== */

[[nodiscard]]
static size_t get_pad_button_index(Input::Gamepad::Buttons button) {
    auto index = static_cast<size_t>(button);
    return index;
}

[[nodiscard]]
static size_t get_pad_axis_index(Input::Gamepad::Axis axis) {
    auto index = static_cast<size_t>(axis);
    return index;
}

bool InputService::pad_event(const EventInput &event) {
    if (const auto *gamepad = std::get_if<EventInputGamepadConnected>(&event)) {
        LOG_INFO("Gamepad device {0} connected (name: {1})", gamepad->pad, gamepad->name);
        auto pad = static_cast<size_t>(gamepad->pad);
        m_pad_connected[pad] = true;
        return true;
    }
    if (const auto *gamepad = std::get_if<EventInputGamepadDisconnected>(&event)) {
        LOG_INFO("Gamepad device {0} disconnected", gamepad->pad);
        auto pad = static_cast<size_t>(gamepad->pad);
        m_pad_connected[pad] = false;
        return true;
    }
    if (const auto *gamepad = std::get_if<EventInputGamepadButton>(&event)) {
        auto pad = static_cast<size_t>(gamepad->pad);
        if (!m_pad_connected[pad]) {
            return true;
        }

        size_t idx = get_pad_button_index(gamepad->button);
        ASSERT(idx < Input::Gamepad::BUTTON_COUNT, "Gamepad button out-of-range");

        uint32_t mask = GET_BIT(idx);
        if (gamepad->pressed) {
            /* turn pad bit on */
            m_pad_state[pad].buttons |= mask;
        } else {
            /* turn pad bit off */
            m_pad_state[pad].buttons &= ~mask;
        }
        return true;
    }
    if (const auto *gamepad = std::get_if<EventInputGamepadAxisMotion>(&event)) {
        auto pad = static_cast<size_t>(gamepad->pad);
        if (!m_pad_connected[pad]) {
            return true;
        }
        size_t axis = get_pad_axis_index(gamepad->axis);
        m_pad_state[pad].axis[axis] = gamepad->value;
        return true;
    }
    return false;
}

bool InputService::is_pad_button_pressed(Input::Gamepad::DeviceID device, Input::Gamepad::Buttons button) const {
    auto pad = static_cast<size_t>(device);
    ASSERT(pad < Input::Gamepad::GAMEPAD_MAX);

    size_t index = get_pad_button_index(button);
    ASSERT(index < Input::Gamepad::BUTTON_COUNT);
    return get_flag_at(m_pad_state[pad].buttons, index);
}

bool InputService::is_pad_button_just_pressed(Input::Gamepad::DeviceID device, Input::Gamepad::Buttons button) const {
    auto pad = static_cast<size_t>(device);
    ASSERT(pad < Input::Gamepad::GAMEPAD_MAX);

    size_t index = get_pad_button_index(button);
    ASSERT(index < Input::Gamepad::BUTTON_COUNT);
    return (get_flag_at(m_pad_state[pad].buttons, index) &&
            !get_flag_at(m_pad_state_previous[pad].buttons, index));
}

bool InputService::is_pad_button_just_released(Input::Gamepad::DeviceID device, Input::Gamepad::Buttons button) const {
    auto pad = static_cast<size_t>(device);
    ASSERT(pad < Input::Gamepad::GAMEPAD_MAX);

    size_t index = get_pad_button_index(button);
    ASSERT(index < Input::Gamepad::BUTTON_COUNT);
    return (!get_flag_at(m_pad_state[pad].buttons, index) &&
            get_flag_at(m_pad_state_previous[pad].buttons, index));
}

Vec2 InputService::get_pad_stick(Input::Gamepad::DeviceID device, bool right) const {
    auto pad = static_cast<size_t>(device);
    ASSERT(pad < Input::Gamepad::GAMEPAD_MAX);

    size_t x_axis, y_axis;
    if (right) {
        x_axis = get_pad_axis_index(Input::Gamepad::Axis::RIGHT_HORIZONTAL);
        y_axis = get_pad_axis_index(Input::Gamepad::Axis::RIGHT_VERTICAL);
    } else {
        x_axis = get_pad_axis_index(Input::Gamepad::Axis::LEFT_HORIZONTAL);
        y_axis = get_pad_axis_index(Input::Gamepad::Axis::LEFT_VERTICAL);
    }

    float x = m_pad_state[pad].axis[x_axis];
    float y = m_pad_state[pad].axis[y_axis];
    return {x, y};
}
