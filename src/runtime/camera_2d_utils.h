#include "core/assertion.h"
#include "core/camera.h"
#include "core/view_context_2d.h"
#include "core/viewport.h"

namespace Camera2DUtils {

[[nodiscard]]
inline static ViewContext2D build_view_context(Camera2D &camera, const Viewport &viewport) noexcept {
    ASSERT(viewport.size.x != 0 && viewport.size.y != 0, "Width & height must not be 0.");
    return {
        .view_camera = &camera,
        .view_size = Vec2(viewport.size),
        .window_viewport = viewport,
    };
}

} // namespace Camera2DUtils
