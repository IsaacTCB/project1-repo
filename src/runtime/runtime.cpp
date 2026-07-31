#include "runtime.h"

#include <algorithm>
#include <memory>
#include <optional>

#include "core/assertion.h"
#include "core/events/input_events.h"
#include "core/events/platform_events.h"
#include "core/events/window_events.h"
#include "core/logger.h"

#include "core/math/color.h"
#include "core/view_context_2d.h"
#include "platform/api/event_server.h"

#include "renderer/api/render_handles.h"
#include "renderer/api/render_queue.h"
#include "renderer/api/renderer.h"

#include "app/application_handler.h"
#include "asset_service.h"
#include "input_service.h"
#include "platform_services.h"
#include "runtime/app/application.h"
#include "runtime/app/context/frame_context.h"
#include "server/display_server.h"
#include "server/os_server.h"

EngineRuntime::EngineRuntime(const PlatformServices &platform)
    : m_platform(platform) {
}

void EngineRuntime::init(IApplicationHandler &app_handler) {
    /* bind with app handler */
    m_app_handler = &app_handler;

    /* initialize fps counter */
    m_fps = 0;
    m_fps_accum = 0;
    m_fps_timer = 0.0f;

    auto &os = m_platform.os;
    LOG_INFO("platform: {}", os.get_platform());
    LOG_INFO("os: {}", os.get_os());

    /* create main window/screen */
    auto &display = m_platform.display;
    DisplayWindowProps props{};
    props.size = {800, 600};
    props.api = RendererAPI::OpenGL;

    m_window = display.init_display(props);
    if (!m_window) {
        LOG_CRITICAL("EngineRuntime::init(): Failed to create main window");
        exit(-1);
    }

    /* initialize renderer */
    m_renderer = Renderer::create(m_window);
    if (!m_renderer) {
        LOG_CRITICAL("EngineRuntime::init(): Failed to create renderer");
        exit(-1);
    }

    /* initialize asset service */
    m_asset_service.emplace(::AssetService::Provider{
        .renderer{
            .load_texture = [](ImageData &image, void *userdata) -> TextureHandle {
                ASSERT(userdata);
                auto &renderer = *static_cast<std::unique_ptr<Renderer> *>(userdata);
                return renderer->load_texture(image);
            },
            .unload_texture = [](TextureHandle texture, void *userdata) {
                ASSERT(userdata);
                auto &renderer = *static_cast<std::unique_ptr<Renderer> *>(userdata);
                renderer->destroy_texture(texture);
                // this comment fixes clang-format. do not remove it
            },
            .userdata = &m_renderer,
        },
    });
    /* poll window size */
    Vec2i display_size = display.get_size(m_window);
    LOG_INFO("display size: {0}", display_size);
    m_input_service.init();

    /* initialize application */
    IApplication::Servers app_servers{
        .input = m_input_service,
        .asset = m_asset_service.value(),
    };
    m_app_handler->init(app_servers, m_platform);
}

bool EngineRuntime::count_fps(float delta) {
    m_fps_timer += delta;
    m_fps_accum++;

    /* if a second has passed,
     * transfer accumulator to fps */
    if (m_fps_timer >= 1.0f) {
        m_fps = m_fps_accum;
        m_fps_accum = {};
        m_fps_timer = {};

        return true;
    }

    return false;
}

void EngineRuntime::update(float delta) {
    /* it's important that we call this
     * before clamping delta time since
     * we want accurate results */
    bool new_second = count_fps(delta);

    /* clamps deltatime to avoid huge skips */
    delta = std::min(delta, 0.25f);

    /* close application if requested */
    if (!m_platform.event.span<EventPlatformExited>().empty()) {
        m_platform.os.exit_app();
        return;
    }

    for (const auto &event : m_platform.event.span<EventWindow>()) {
        if (const auto *ptr = std::get_if<EventWindowResized>(&event)) {
            LOG_TRACE("resized: {}", ptr->size);

            /* skip invalid window sizes for rendering */
            if (ptr->size.x <= 0 || ptr->size.y <= 0) {
                continue;
            }
            m_renderer->on_screen_resized(ptr->size);
        }
        if (std::get_if<EventWindowMaximized>(&event)) {
            LOG_TRACE("maximized");
        }
        if (std::get_if<EventWindowMinimized>(&event)) {
            LOG_TRACE("minimized");
        }
    }

    m_input_service.update();

    for (const auto &event : m_platform.event.span<EventInput>()) {
        m_input_service.event(event);
    }

    /* update it */
    m_view_ctx = {};
    UpdateContext update_ctx{
        .frame = build_frame_context(delta),
        .view_context = m_view_ctx,
        .new_second = new_second,
    };
    m_app_handler->update(update_ctx);
}

void EngineRuntime::render() {
    m_queue.clear();

    /* build render frame! */
    RenderContext render{
        .frame = m_frame_ctx,
        .view_context = m_view_ctx,
        .queue = m_queue,
        .clear_color = Color{25, 25, 25, 0},
    };

    ASSERT(m_view_ctx.view_camera, "View context must be built every frame!");
    m_app_handler->render(render);

    render_frame(render);
}

void EngineRuntime::render_frame(RenderContext &ctx) {
    m_renderer->clear(ctx.clear_color);
    m_renderer->render_pass(ctx.view_context, ctx.queue);
    m_renderer->swap_buffers();
}

void EngineRuntime::close() {
    m_app_handler->close();

    m_input_service.close();
    m_asset_service.reset();

    m_renderer.reset();
    m_platform.display.close_display(m_window);
}

FrameContext &EngineRuntime::build_frame_context(float delta) {
    m_frame_ctx = {
        .delta = delta,
        .fps = m_fps,
        .viewport = m_renderer->get_absolute_viewport({
            .position = {0.0f, 0.0f},
            .size = {1.0f, 1.0f},
        }),
    };
    return m_frame_ctx;
}
