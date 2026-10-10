#pragma once

#include "core/math/vec2.h"
#include "core/math/vec2i.h"
#include "core/view_layout.h"
#include "core/viewport.h"

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
    void set_output_rect(const Viewport &rect) {
        if (rect.position == m_output_rect.position && rect.size == m_output_rect.size) {
            return;
        }
        m_output_rect = rect;
        m_dirty = true;
    }

    void set_presentation(const ViewPresentationProps &presentation) {
        m_presentation = presentation;
        m_dirty = true;
    }

    const ViewLayout &get_layout() {
        if (m_dirty) {
            calculate_layout();
            m_dirty = false;
        }
        return m_layout;
    }

private:
    void calculate_layout();

private:
    ViewPresentationProps m_presentation;
    Viewport m_output_rect{};

    ViewLayout m_layout;
    bool m_dirty = true;
};
