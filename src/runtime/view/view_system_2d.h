#pragma once

#include "core/math/vec2.h"
#include "core/math/vec2i.h"
#include "core/view_layout.h"

#include <cstdint>

enum class PresentationMode : uint8_t {
    DISABLED,
    FIXED_RESOLUTION,
    LOGICAL_VIEW,
};

enum class AspectPolicy : uint8_t {
    IGNORE,
    KEEP_WIDTH,
    KEEP_HEIGHT,
    EXPAND,
};

struct ViewPresentationProps {
    Vec2 logical_size = {800.0f, 600.0f};
    PresentationMode mode = PresentationMode::DISABLED;
    AspectPolicy aspect = AspectPolicy::IGNORE;
};

class ViewSystem {
public:
    void set_output_size(Vec2i size) {
        m_output_size = size;
    }

    void set_presentation(const ViewPresentationProps &presentation) {
        m_presentation = presentation;
    }

    ViewLayout calculate_layout();

private:
    ViewPresentationProps m_presentation;
    Vec2i m_output_size;
};
