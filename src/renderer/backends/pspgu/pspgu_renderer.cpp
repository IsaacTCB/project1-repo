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

#include "pspgu_consts.h"
#include "renderer/backends/pspgu/pspgu_vertex.h"

struct PSPGuRenderer::Impl {
    inline static std::array<vram_ptr, 2> s_framebuffers;
    alignas(64) inline static char s_display_list[PSP_DISPLAYLIST_SIZE];
};

PSPGuRenderer::PSPGuRenderer([[maybe_unused]] DisplayWindowHandle &window_data) {
    sceGuInit();

    /* init framebuffers */
    for (auto &fb : Impl::s_framebuffers) {
        /* TODO: replace this with pspsdk's vram library */
        fb = guGetStaticVramBuffer(
            PSP_FRAMEBUFFER_WIDTH,
            PSP_FRAMEBUFFER_HEIGHT,
            PSP_FRAMEBUFFER_PSM);
    }

    sceGuStart(GU_DIRECT, Impl::s_display_list);
    sceGuDrawBuffer(PSP_FRAMEBUFFER_PSM, Impl::s_framebuffers[0], PSP_FRAMEBUFFER_WIDTH);
    sceGuDispBuffer(PSP_SCREEN_WIDTH, PSP_SCREEN_HEIGHT, Impl::s_framebuffers[1], PSP_FRAMEBUFFER_WIDTH);

    /* disable depth buffer */
    sceGuDepthBuffer(Impl::s_framebuffers[0], 0);
    sceGuDisable(GU_DEPTH_TEST); // Disable depth testing

    /* setup viewport */
    sceGuOffset(2048 - (PSP_SCREEN_WIDTH / 2), 2048 - (PSP_SCREEN_HEIGHT / 2));
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuViewport(2048, 2048, PSP_SCREEN_WIDTH, PSP_SCREEN_HEIGHT);
    sceGuScissor(0, 0, PSP_SCREEN_WIDTH, PSP_SCREEN_HEIGHT);

    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

    apply_viewport({
        .position = {0.0f, 0.0f},
        .size = {
            static_cast<float>(PSP_SCREEN_WIDTH),
            static_cast<float>(PSP_SCREEN_HEIGHT),
        },
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

    /* start a new frame and enable the display */
    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
    sceGuDisplay(GU_TRUE);
}

PSPGuRenderer::~PSPGuRenderer() {
    sceGuDisplay(GU_FALSE);
    sceGuTerm();
}

void PSPGuRenderer::clear(Color color) {
    sceGuStart(GU_DIRECT, Impl::s_display_list);

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
    ScePspFVector3 rot{
        .x = 0.0f,
        .y = 0.0f,
        .z = camera->rotation,
    };
    sceGumRotateXYZ(&rot);

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
    sceGuStart(GU_DIRECT, Impl::s_display_list);

    /* apply view matrices */
    sceGumMatrixMode(GU_PROJECTION);
    apply_camera_projection(view.view_camera, view.view_size);

    sceGumMatrixMode(GU_VIEW);
    apply_camera_view(view.view_camera);

    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();

    /* draw something here */
    for (const auto &command : queue.get()) {
        std::visit([this](const auto &cmd) {
            this->command(cmd);
        },
            command);
    }

    /* finish frame */
    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
};

void PSPGuRenderer::apply_viewport(const Viewport &viewport) {
    m_screen_viewport = viewport;
}

void PSPGuRenderer::command(const RenderCommandDraw &command) {
    uint32_t color = GU_RGBA(
        command.color.r,
        command.color.g,
        command.color.b,
        command.color.a);

    auto *vertices = static_cast<PSPGuVertex *>(
        sceGuGetMemory(6 * sizeof(PSPGuVertex)));

    vertices[0] = {
        .color = color,
        .position{
            .x = command.x,
            .y = command.y,
            .z = 0.0f,
        },
    };
    vertices[1] = {
        .color = color,
        .position{
            .x = command.x + command.width,
            .y = command.y,
            .z = 0.0f,
        },
    };
    vertices[2] = {
        .color = color,
        .position{
            .x = command.x + command.width,
            .y = command.y + command.height,
            .z = 0.0f,
        },
    };

    vertices[3] = {
        .color = color,
        .position{
            .x = command.x + command.width,
            .y = command.y + command.height,
            .z = 0.0f,
        },
    };
    vertices[4] = {
        .color = color,
        .position{
            .x = command.x,
            .y = command.y + command.height,
            .z = 0.0f,
        },
    };
    vertices[5] = {
        .color = color,
        .position{
            .x = command.x,
            .y = command.y,
            .z = 0.0f,
        },
    };

    sceGumDrawArray(GU_TRIANGLES, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, 6, nullptr, vertices);
}
