#pragma once

#include <cstddef>
#include <cstdint>

namespace Input::Mouse {

enum class Buttons : uint8_t {
    UNKNOWN = 0,

    LEFT = 1,
    MIDDLE = 2,
    RIGHT = 3,

    EXTRA1 = 4,
    EXTRA2 = 5,

    COUNT = 6,
};

constexpr auto BUTTON_COUNT = static_cast<size_t>(::Input::Mouse::Buttons::COUNT);

} // namespace Input::Mouse
