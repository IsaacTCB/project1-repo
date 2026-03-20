#include "gl_renderer.h"
#include "gl_context.h"

#include "core/logger.h"

#include <glad/gl.h>

static bool s_glad_loaded = false;

struct GLRenderer::Impl {
    GLContext context;
};

static bool init_glad(GLADloadfunc func) {
    int version = gladLoadGL(func);
    return (version != 0);
}

GLRenderer::GLRenderer(DisplayWindowHandle& window_data)
    :m_impl(std::make_unique<Impl>()) {
    m_impl->context.init(window_data.raw_window_handle);
    if (!s_glad_loaded) {
        s_glad_loaded = true;
        bool success = init_glad(
            static_cast<GLADloadfunc>(m_impl->context.load_func)
        );
        if (!success) {
            LOG_CRITICAL("Glad failed to load OpenGL.");
        }
    }
}

GLRenderer::~GLRenderer() = default;

void GLRenderer::clear(float r, float g, float b, float a) {
    m_impl->context.bind();
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void GLRenderer::swap_buffers() {
    m_impl->context.swap_buffers();
}
