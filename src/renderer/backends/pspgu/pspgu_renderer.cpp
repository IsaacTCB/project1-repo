#include "pspgu_renderer.h"

#include <array>
#include <cstdint>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <psptypes.h>
#include <psputils.h>
#include <utility>

#include "core/assertion.h"
#include "core/camera.h"
#include "core/handles.h"
#include "core/view_context_2d.h"
#include "renderer/api/render_handles.h"

#include "pspgu_consts.h"
#include "pspgu_texture.h"
#include "pspgu_vertex.h"

struct PSPGuRenderer::Impl {
    inline static std::array<vram_ptr, 2> s_framebuffers;
    alignas(64) inline static char s_display_list[PSP_DISPLAYLIST_SIZE];

    /* used to know if a texture is already active or not.
     * null for solid color, anything else for texture buffer */
    inline static const uint8_t *s_active_texture{};

    /* handles */
    HandlePool<PSPGuTexture> textures;
};

PSPGuRenderer::PSPGuRenderer([[maybe_unused]] DisplayWindowHandle &window_data) {
    m_impl.reset(new Impl());

    sceGuInit();
    gumInit();

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
    sceGuViewport(2048, 2048, PSP_SCREEN_WIDTH, -PSP_SCREEN_HEIGHT);
    sceGuScissor(0, 0, PSP_SCREEN_WIDTH, PSP_SCREEN_HEIGHT);

    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

    m_screen_viewport = {
        .position = {0.0f, 0.0f},
        .size = {
            static_cast<float>(PSP_SCREEN_WIDTH),
            static_cast<float>(PSP_SCREEN_HEIGHT),
        },
    };

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

void PSPGuRenderer::on_screen_resized([[maybe_unused]] const Vec2i &screen_size) {};

TextureHandle PSPGuRenderer::load_texture(ImageData &image) {
    /* TODO: once proper handle error handling gets added, then
     * this should fail safely if image is larger than 512x512 */
    PSPGuImageBuffer converted = convert_image(image);
    sceKernelDcacheWritebackInvalidateAll();

    /* calculate logical ratio (aka the ratio between
     * the texture logical size and its "power of 2" size) */
    Vec2 logical{Vec2i{(int)image.width, (int)image.height}};
    logical /= Vec2{Vec2i{(int)converted.width, (int)converted.height}};

    return m_impl->textures.emplace(PSPGuTexture{
        .buffer = std::move(converted),
        .logical_ratio = logical,
        .logical_width = (uint16_t)image.height,
        .logical_height = (uint16_t)image.width,
        .filter = false,
    });
};

void PSPGuRenderer::destroy_texture(TextureHandle handle) {
    auto texture = m_impl->textures.get_and_destroy(handle);

    /* realistically, this won't be necessary */
    if (texture->buffer.data() == Impl::s_active_texture) {
        unbind_texture();
    }
}

void PSPGuRenderer::render_pass(const ViewContext2D &view, const RenderQueue &queue) {
    /* start frame and start matrices */
    sceGuStart(GU_DIRECT, Impl::s_display_list);
    load_matrices(view);

    /* draw objects */
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

void PSPGuRenderer::bind_texture(const PSPGuTexture &texture) {
    const PSPGuImageBuffer &image_buffer = texture.buffer;

    if (image_buffer.data() == Impl::s_active_texture) {
        /* this texture is already active */
        return;
    }

    /* re-enable texture 2d if texture was unbound */
    if (!Impl::s_active_texture) {
        sceGuEnable(GU_TEXTURE_2D);
    }

    /* apply texture props */
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    sceGuTexWrap(GU_REPEAT, GU_REPEAT);

    int filter = texture.filter ? GU_LINEAR : GU_NEAREST;
    sceGuTexFilter(filter, filter);

    ASSERT(((uintptr_t)image_buffer.data()) % 16 == 0, "PSPGuRenderer::bind_texture: texture data is not 16-byte aligned!");
    sceGuTexMode(image_buffer.format, 0, 0, GU_FALSE);
    sceGuTexImage(0, image_buffer.width, image_buffer.height, image_buffer.width, image_buffer.data());

    Impl::s_active_texture = image_buffer.data();
}

void PSPGuRenderer::unbind_texture() {
    /* disables textures, forcing GU
     * to draw solid colors */
    if (Impl::s_active_texture) {
        sceGuDisable(GU_TEXTURE_2D);
        Impl::s_active_texture = nullptr;
    }
}

void PSPGuRenderer::command(const RenderCommandDraw &command) {
    constexpr size_t QUAD_VERTEX_COUNT = 4;
    constexpr size_t QUAD_INDEX_COUNT = sizeof(g_quad_indices) / sizeof(g_quad_indices[0]);

    /* convert RGBA to ABGR */
    uint32_t color = GU_RGBA(
        command.color.r,
        command.color.g,
        command.color.b,
        command.color.a);

    Vec2 uv_min = Vec2{0.0f, 0.0f};
    Vec2 uv_max = Vec2{1.0f, 1.0f};

    if (command.texture.has_value()) {
        /* fetch texture */
        TextureHandle handle = command.texture.value();
        auto *texture = m_impl->textures.get(handle);
        ASSERT(texture, "PSPGuRenderer::Command(RenderCommandDraw): failed to locate texture");

        if ((texture->logical_width != texture->buffer.width) ||
            (texture->logical_height != texture->buffer.height)) {
            /* update uv coords */
            uv_min *= texture->logical_ratio;
            uv_max *= texture->logical_ratio;
        }

        bind_texture(*texture);
    } else {
        /* this is a solid color */
        unbind_texture();
    }

    auto *vertices = static_cast<PSPGuVertex *>(
        sceGuGetMemory(sizeof(PSPGuVertex) * QUAD_VERTEX_COUNT));

    /* create quad mesh */
    vertices[0] = {
        .u = uv_min.x,
        .v = uv_min.y,
        .color = color,
        .position{
            .x = command.x,
            .y = command.y,
            .z = 0.0f,
        },
    };
    vertices[1] = {
        .u = uv_max.x,
        .v = uv_min.y,
        .color = color,
        .position{
            .x = command.x + command.width,
            .y = command.y,
            .z = 0.0f,
        },
    };
    vertices[2] = {
        .u = uv_max.x,
        .v = uv_max.y,
        .color = color,
        .position{
            .x = command.x + command.width,
            .y = command.y + command.height,
            .z = 0.0f,
        },
    };
    vertices[3] = {
        .u = uv_min.x,
        .v = uv_max.y,
        .color = color,
        .position{
            .x = command.x,
            .y = command.y + command.height,
            .z = 0.0f,
        },
    };

    /* build flag list */
    int quad_flags{};
    quad_flags |= GU_INDEX_8BIT;
    quad_flags |= GU_TEXTURE_32BITF;
    quad_flags |= GU_COLOR_8888;
    quad_flags |= GU_VERTEX_32BITF;
    quad_flags |= GU_TRANSFORM_3D;

    /* draw our quad */
    sceGumDrawArray(
        GU_TRIANGLES,
        quad_flags,
        QUAD_INDEX_COUNT,
        g_quad_indices,
        vertices);
}

/* matrix functions */

static void matrix_load_projection(const ViewContext2D &view) {
    auto ortho = view.get_ortho();
    sceGumOrtho(ortho.tl.x, ortho.br.x, ortho.tl.y, ortho.br.y, -1.0f, 1.0f);
}

static void matrix_load_view(const ViewContext2D &view) {
    const Camera2D *camera = view.view_camera;

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

static constexpr auto get_matrix_pipeline() {
    using PipelineLoadFn = void (*)(const ViewContext2D &view);
    using PipelineStep = std::pair<int, PipelineLoadFn>;

    /* first value is matrix mode, second is its load function */
    return std::to_array<PipelineStep>({
        {GU_PROJECTION, matrix_load_projection},
        {GU_VIEW, matrix_load_view},
        {GU_MODEL, nullptr},
    });
}

void PSPGuRenderer::load_matrices(const ViewContext2D &view) {
    constexpr auto pipeline = get_matrix_pipeline();
    for (const auto &[matrix_mode, matrix_func] : pipeline) {
        sceGumMatrixMode(matrix_mode);
        sceGumLoadIdentity();

        /* load the view matrix */
        if (matrix_func) {
            matrix_func(view);
        }
    }
}
