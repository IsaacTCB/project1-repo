#pragma once

#include <memory>

#include "platform/api/display_server.h"

class Renderer {
public:
    static std::shared_ptr<Renderer> create(DisplayWindowHandle& window_data);

public:
    virtual ~Renderer() = default;

    virtual void clear(float r, float g, float b, float a = 255) = 0;
    virtual void swap_buffers() = 0;

protected:
    Renderer() = default;
};
