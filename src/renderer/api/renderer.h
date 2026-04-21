#pragma once

#include <memory>

#include "core/image/image.h"
#include "platform/api/display_server.h"

#include "render_handles.h"
#include "render_queue.h"

class Renderer {
public:
    static std::shared_ptr<Renderer> create(DisplayWindowHandle &window_data);

public:
    virtual ~Renderer() = default;
    Renderer(const Renderer &other) = delete;
    Renderer operator=(const Renderer &other) = delete;
    Renderer(Renderer &&rhs) = delete;
    Renderer operator=(Renderer &&rhs) = delete;

    virtual void clear(float r, float g, float b, float a = 255) = 0;
    virtual void swap_buffers() = 0;

    virtual TextureHandle load_texture(ImageData &image) = 0;
    virtual void destroy_texture(TextureHandle texture) = 0;

    virtual void render_pass(const RenderQueue &queue) = 0;

protected:
    Renderer() = default;
};
