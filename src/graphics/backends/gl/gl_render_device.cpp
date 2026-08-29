#include "gl_render_device.h"

#include "core/logger.h"
#include "core/viewport.h"
#include "gl_context.h"

#include <SDL3/SDL_video.h>
#include <glad/gl.h>

static bool s_glad_loaded = false;

static bool init_glad(GLADloadfunc func) {
    int version = gladLoadGL(func);
    return (version != 0);
}

GL_RenderDevice::GL_RenderDevice(std::unique_ptr<GL_IContext> &&context)
    : m_ctx(std::move(context)) {
    m_ctx->get_load_proc();

    if (!s_glad_loaded) {
        s_glad_loaded = true;
        bool success = init_glad(
            static_cast<GLADloadfunc>(m_ctx->get_load_proc()));
        if (!success) {
            LOG_CRITICAL("gl: Glad failed to load OpenGL.");
        }

        print_info();
    }

    /* This allows gl to load images whose
     * data size are not a multiple of 4. */
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

GL_RenderDevice::~GL_RenderDevice() = default;

/* ================= *
 *      Frames       *
 * ================= */

void GL_RenderDevice::begin_frame() {}

void GL_RenderDevice::end_frame() {
    m_ctx->swap_buffers();
}

/* ================= *
 *     Commands      *
 * ================= */

void GL_RenderDevice::clear([[maybe_unused]] const Color &color) {
    const auto [r, g, b, a] = color.normalized();
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

Viewport GL_RenderDevice::build_viewport(const Vec2i &size) {
    return Viewport{
        .position{},
        .size{size},
    };
}

void GL_RenderDevice::apply_viewport(const Viewport &screen_viewport, const Viewport &window_viewport) {
    /* TODO: Y flipping is kinda annoying */
    glViewport(
        static_cast<GLint>(window_viewport.position.x),
        static_cast<GLint>(
            /* flips the Y axis cuz gl... */
            screen_viewport.size.y - (window_viewport.position.y + window_viewport.size.y)),
        static_cast<GLint>(window_viewport.size.x),
        static_cast<GLint>(window_viewport.size.y));
}

Vec2i GL_RenderDevice::get_screen_size() {
    return m_ctx->get_screen_size();
}

/* ================= *
 *       Utils       *
 * ================= */

void GL_RenderDevice::print_info() {
    const char *vendor = (const char *)glGetString(GL_VENDOR);
    LOG_INFO("gl: GPU Vendor: {}", vendor);

    const char *renderer = (const char *)glGetString(GL_RENDERER);
    LOG_INFO("gl: GPU Renderer: {}", renderer);

    const char *version = (const char *)glGetString(GL_VERSION);
    LOG_INFO("gl: OpenGL Version: {}", version);

    const char *glsl_version = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
    LOG_INFO("gl: GLSL Version: {}", glsl_version);
}
