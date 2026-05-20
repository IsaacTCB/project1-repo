#include "core/assertion.h"
#include "core/camera.h"
#include "core/view_context_2d.h"
#include "core/viewport.h"

namespace Camera2DUtils {

[[nodiscard]]
inline static ViewContext2D build_view_context(Camera2D &camera, const Viewport &viewport) noexcept {
    ASSERT(viewport.width != 0 && viewport.height != 0, "Width & height must not be 0.");
    return {
        .view_camera = &camera,
        .view_width = viewport.width,
        .view_height = viewport.height,
        .window_viewport = viewport,
    };
}

} // namespace Camera2DUtils
