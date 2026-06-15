#include "pspgu_renderer.h"

#include <array>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <psptypes.h>

#include "core/camera.h"
#include "core/view_context_2d.h"
#include "core/viewport.h"
#include "renderer/api/render_handles.h"

constexpr int PSP_SCREEN_WIDTH = 480;
constexpr int PSP_SCREEN_HEIGHT = 272;
constexpr int PSP_FRAMEBUFFER_WIDTH = 512;
constexpr int PSP_FRAMEBUFFER_HEIGHT = 272;

struct PSPGuRenderer::Impl {
    inline static std::array<void *, 2> s_framebuffers;
    alignas(64) inline static char s_list[0x20000];
};

PSPGuRenderer::PSPGuRenderer([[maybe_unused]] DisplayWindowHandle &window_data) {
    sceGuInit();

    for (auto &fb : Impl::s_framebuffers) {
        /* TODO: replace this with pspsdk's vram library */
        fb = guGetStaticVramBuffer(
            PSP_FRAMEBUFFER_WIDTH,
            PSP_FRAMEBUFFER_HEIGHT,
            GU_PSM_8888);
    }

    // init framebuffers
    sceGuStart(GU_DIRECT, Impl::s_list);
    sceGuDrawBuffer(GU_PSM_8888, Impl::s_framebuffers[0], PSP_FRAMEBUFFER_WIDTH);
    sceGuDispBuffer(PSP_SCREEN_WIDTH, PSP_SCREEN_HEIGHT, Impl::s_framebuffers[1], PSP_FRAMEBUFFER_WIDTH);

    /* disable depth buffer */
    sceGuDepthBuffer(Impl::s_framebuffers[0], 0);
    sceGuDisable(GU_DEPTH_TEST); // Disable depth testing

    /* setup viewport */
    sceGuOffset(2048 - (PSP_SCREEN_WIDTH / 2), 2048 - (PSP_SCREEN_HEIGHT / 2));
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuViewport(2048, 2048, PSP_SCREEN_WIDTH, PSP_SCREEN_HEIGHT);
    sceGuScissor(0, 0, PSP_SCREEN_WIDTH, PSP_SCREEN_HEIGHT);
    apply_viewport({
        .position = {0.0f, 0.0f},
        .size = {480.0f, 272.0f},
    });

    /* init matrices */
    constexpr std::array MATRIX_MODES{
        GU_PROJECTION,
        GU_VIEW,
        GU_MODEL,
    };
    for (auto matrix_mode : MATRIX_MODES) {
        sceGumMatrixMode(matrix_mode);
        sceGumLoadIdentity();
    }

    // Start a new frame and enable the display
    sceGuFinish();
    sceGuDisplay(GU_TRUE);
}

PSPGuRenderer::~PSPGuRenderer() {
    sceGuDisplay(GU_FALSE);
    sceGuTerm();
}

void PSPGuRenderer::clear(Color color) {
    sceGuStart(GU_DIRECT, Impl::s_list);

    sceGuClearColor(GU_RGBA(color.r, color.g, color.b, color.a));
    sceGuClear(GU_COLOR_BUFFER_BIT);

    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
};

void PSPGuRenderer::swap_buffers() {
    sceDisplayWaitVblankStart();
    sceGuSwapBuffers();
};

void PSPGuRenderer::on_screen_resized(const Vec2i &screen_size) {};

TextureHandle PSPGuRenderer::load_texture(ImageData &image) {
    return TextureHandle{};
};

void PSPGuRenderer::destroy_texture(TextureHandle handle) {}

static void apply_camera_projection(Camera2D *camera, const Vec2 &size) {
    sceGumLoadIdentity();

    float left, right, top, bottom;
    if (camera->anchor == Camera2D::AnchorMode::CENTER) {
        float width_half = size.x * 0.5f;
        float height_half = size.y * 0.5f;

        left = -width_half;
        right = width_half;
        bottom = height_half; /* flip it cuz opengl... */
        top = -height_half;
    } else {
        left = 0.0f;
        right = size.x;
        bottom = size.y; /* same as above */
        top = 0.0f;
    }

    float zoom = 1.0f / std::max(camera->zoom, 0.0001f);
    left *= zoom;
    right *= zoom;
    top *= zoom;
    bottom *= zoom;

    sceGumOrtho(left, right, bottom, top, -1.0f, 1.0f);
}

static void apply_camera_view(Camera2D *camera) {
    sceGumLoadIdentity();

    /* translate to view */
    ScePspFVector3 vec{
        .x = camera->position.x,
        .y = camera->position.y,
        .z = 0.0f,
    };
    sceGumTranslate(&vec);

    /* rotate view */
    sceGumRotateZ(camera->rotation);

    /* apply offset */
    ScePspFVector3 offset{
        .x = camera->offset.x,
        .y = camera->offset.y,
        .z = 0.0f,
    };
    sceGumTranslate(&offset);

    /* reverse it! */
    sceGumFullInverse();
}

void PSPGuRenderer::render_pass(const ViewContext2D &view, const RenderQueue &queue) {
    /* start frame */
    sceGuStart(GU_DIRECT, Impl::s_list);

    /* apply view matrices */
    sceGumMatrixMode(GU_PROJECTION);
    apply_camera_projection(view.view_camera, view.view_size);

    sceGumMatrixMode(GU_VIEW);
    apply_camera_view(view.view_camera);

    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();

    /* draw something here */

    /* finish frame */
    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
};

void PSPGuRenderer::apply_viewport(const Viewport &viewport) {
    m_screen_viewport = viewport;
}
