#pragma once

#include <memory>

#include "core/camera.h"
#include "core/image/image.h"
#include "core/viewport.h"
#include "platform/api/display_server.h"

#include "render_handles.h"
#include "render_queue.h"
#include "render_view.h"

class Renderer {
public:
    static std::unique_ptr<Renderer> create(DisplayWindowHandle &window_data);

public:
    virtual ~Renderer() = default;
    Renderer(const Renderer &other) = delete;
    Renderer operator=(const Renderer &other) = delete;
    Renderer(Renderer &&rhs) = delete;
    Renderer operator=(Renderer &&rhs) = delete;

    RenderView get_render_view(ICamera &target_camera, const Viewport &viewport) const;

public:
    virtual void clear(float r, float g, float b, float a = 255) = 0;
    virtual void swap_buffers() = 0;
    virtual void on_screen_resized(uint32_t width, uint32_t height) = 0;

    virtual TextureHandle load_texture(ImageData &image) = 0;
    virtual void destroy_texture(TextureHandle texture) = 0;

    virtual void render_pass(const RenderView &view, const RenderQueue &queue) = 0;

protected:
    Viewport m_screen_viewport;
    Renderer() = default;
};
