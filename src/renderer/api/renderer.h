#pragma once

#include <memory>

#include "core/handles.h"
#include "core/image/image.h"

#include "platform/api/display_server.h"

using TextureHandle = Handle;

class Renderer {
public:
    static std::shared_ptr<Renderer> create(DisplayWindowHandle& window_data);

public:
    virtual ~Renderer() = default;
    Renderer(const Renderer& other) = delete;
    Renderer operator=(const Renderer& other) = delete;
    Renderer(Renderer&& rhs) = delete;
    Renderer operator=(Renderer&& rhs) = delete;

    virtual void clear(float r, float g, float b, float a = 255) = 0;
    virtual void swap_buffers() = 0;

    virtual TextureHandle load_texture(ImageData& image) = 0;
    virtual void destroy_texture(TextureHandle texture) = 0;

    virtual void draw_texture(TextureHandle texture, int x, int y) = 0;

protected:
    Renderer() = default;
};
