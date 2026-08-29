#pragma once

#include "core/math/color.h"
#include "core/math/vec2.h"
#include "core/math/vec2i.h"
#include "core/viewport.h"

class IRenderDevice {
public:
    virtual ~IRenderDevice() = default;

protected:
    IRenderDevice() = default;
    friend class Renderer;

    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;

    virtual void clear(const Color &color) = 0;

    virtual Viewport build_viewport(const Vec2i &size) = 0;
    virtual void apply_viewport(const Viewport &screen_viewport, const Viewport &window_viewport) = 0;

    /* currently only used for fetching the screen size when the renderer device gets attached. */
    virtual Vec2i get_screen_size() = 0;
};
