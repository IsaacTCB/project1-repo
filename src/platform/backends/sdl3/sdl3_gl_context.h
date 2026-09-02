#pragma once

#include "core/math/vec2i.h"
#include "graphics/backends/gl/gl_context.h"
#include "graphics/backends/gl/gl_render_device.h"

#include <memory>

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_video.h>

class SDL3_GL_Context final : public GL_IContext {
public:
    SDL3_GL_Context(SDL_Window *window);
    ~SDL3_GL_Context() override;

    void bind() override;
    FnLoad get_load_proc() override;
    void swap_buffers() override;
    void set_swap_interval(int interval) override;
    Vec2i get_screen_size() override;
    bool is_extension_supported(std::string_view extension_name) override;

private:
    SDL_Window *m_window;
    SDL_GLContext m_context;
};

class SDL3_GL_RenderDeviceFactory : public GL_IRenderDeviceFactory {
public:
    SDL3_GL_RenderDeviceFactory(SDL_Window *window)
        : m_window(window) {};

protected:
    std::unique_ptr<GL_IContext> get_context() override {
        return std::unique_ptr<GL_IContext>(new SDL3_GL_Context(m_window));
    }

private:
    SDL_Window *m_window;
};
