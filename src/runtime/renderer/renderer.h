#pragma once

#include <memory>

#include "core/image/image.h"
#include "core/math/vec2i.h"
#include "core/viewport.h"

#include "render_handles.h"

#include "runtime/app/context/frame_context.h"

class IRenderDevice;
class IRenderDeviceFactory;

class Renderer {
public:
    Renderer();
    ~Renderer();

public:
    bool build_device(std::unique_ptr<IRenderDeviceFactory> factory);
    void close_device();

public:
    void render_pass(const RenderContext &ctx);

    TextureHandle load_texture(ImageData &data);
    void destroy_texture(TextureHandle texture);

    [[nodiscard]]
    const Viewport &get_viewport() const { return m_screen_viewport; };

    void on_screen_resized(const Vec2i &screen_size);

private:
    std::unique_ptr<IRenderDevice> m_rd;
    Viewport m_screen_viewport;
};
