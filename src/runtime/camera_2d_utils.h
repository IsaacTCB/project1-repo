#include "core/camera.h"
#include "core/view_context_2d.h"
#include "core/view_layout.h"

namespace Camera2DUtils {

[[nodiscard]]
inline static ViewContext2D build_view_context(Camera2D &camera, const ViewLayout &layout) noexcept {
    return {
        .view_camera = &camera,
        .layout = layout,
    };
}

} // namespace Camera2DUtils
