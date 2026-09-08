#include "view_system_2d.h"
#include "core/logger.h"

#include <cmath>

static Vec2 calculate_logical_size(Vec2i output_size, const ViewPresentationProps &settings);
static Vec2i calculate_render_target_size(Vec2i output_size, Vec2 logical_size, const ViewPresentationProps &settings);
static Viewport calculate_presentation_rect(Vec2i output_size, Vec2i render_target_size, const ViewPresentationProps &settings);

ViewLayout ViewSystem::calculate_layout() {
    ViewLayout result;
    result.logical_size = calculate_logical_size(
        m_output_size,
        m_presentation);

    result.render_target_size = calculate_render_target_size(
        m_output_size,
        result.logical_size,
        m_presentation);

    result.render_viewport = {
        .position = {0, 0},
        .size = result.render_target_size,
    };

    result.presentation_rect = calculate_presentation_rect(
        m_output_size,
        result.render_target_size,
        m_presentation);

    result.scale = Vec2{result.presentation_rect.size} /
                   Vec2{result.render_target_size};

    return result;
}

/* private */

static Vec2 calculate_logical_size(Vec2i output_size, const ViewPresentationProps &settings) {
    const auto output_width = static_cast<float>(output_size.x);
    const auto output_height = static_cast<float>(output_size.y);

    if (settings.mode == PresentationMode::DISABLED) {
        return Vec2{output_size};
    }

    const float output_aspect =
        output_width / output_height;

    Vec2 result;
    switch (settings.aspect) {
        case AspectPolicy::KEEP_WIDTH:
            result = {
                settings.logical_size.x,
                settings.logical_size.x / output_aspect,
            };
            break;

        case AspectPolicy::KEEP_HEIGHT:
            result = {
                settings.logical_size.y * output_aspect,
                settings.logical_size.y,
            };
            break;

        case AspectPolicy::EXPAND: {
            const float logical_aspect =
                settings.logical_size.x / settings.logical_size.y;

            if (output_aspect > logical_aspect) {
                /* if screen is wider then expose more horizontally */
                result = {
                    settings.logical_size.y * output_aspect,
                    settings.logical_size.y,
                };
                break;
            }

            /* if screen is taller then expose more vertically */
            result = {
                settings.logical_size.x,
                settings.logical_size.x / output_aspect,
            };
            break;
        }

        case AspectPolicy::IGNORE:
            result = settings.logical_size;
            break;
    }

    /* fixed resolution must be divisible by 2 */
    if (settings.mode == PresentationMode::FIXED_RESOLUTION) {
        result = {
            std::ceil(result.x / 2.0f) * 2.0f,
            std::ceil(result.y / 2.0f) * 2.0f,
        };
    }

    return result;
}

static Vec2i calculate_render_target_size(Vec2i output_size, Vec2 logical_size, const ViewPresentationProps &settings) {
    switch (settings.mode) {
        case PresentationMode::FIXED_RESOLUTION: {
            return {
                static_cast<int>(std::ceil(logical_size.x)),
                static_cast<int>(std::ceil(logical_size.y)),
            };
        }

        case PresentationMode::LOGICAL_VIEW:
        case PresentationMode::DISABLED:
            return output_size;
    }

    return output_size;
}

static Viewport calculate_presentation_rect(Vec2i output_size, Vec2i render_target_size, const ViewPresentationProps &settings) {
    if (settings.mode == PresentationMode::DISABLED ||
        settings.aspect == AspectPolicy::IGNORE) {
        return Viewport{
            .position = {0, 0},
            .size = {output_size.x, output_size.y},
        };
    }

    const Vec2 ratio = Vec2{output_size} / Vec2{render_target_size};

    float scale;
    if (settings.aspect == AspectPolicy::KEEP_WIDTH) {
        scale = ratio.x;
    } else if (settings.aspect == AspectPolicy::KEEP_HEIGHT) {
        scale = ratio.y;
    } else {
        scale = std::min(ratio.x, ratio.y);
    }

    const Vec2i size = Vec2i{Vec2{render_target_size} * scale};
    const Vec2i pos = (output_size - size) / 2;

    return {
        .position = pos,
        .size = size,
    };
}
