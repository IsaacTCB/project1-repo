#pragma once

#include <glaze/core/common.hpp>
#include <glaze/forward.hpp>

#include "core/math/vec2.h"
#include "core/math/vec2i.h"

struct Rect {
    int x, y, w, h;
};

template <>
struct glz::meta<Rect> {
    using T = Rect;
    static constexpr auto value = glz::array(&T::x, &T::y, &T::w, &T::h);
};

template <>
struct glz::meta<Vec2> {
    using T = Vec2;
    static constexpr auto value =
        glz::array(&T::x, &T::y);
};

template <>
struct glz::meta<Vec2i> {
    using T = Vec2i;
    static constexpr auto value =
        glz::array(&T::x, &T::y);
};
