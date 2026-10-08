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
#include "runtime/debug/sinks/file_log_sink.h"
#include "server/display_server.h"
#include "server/os_server.h"

EngineRuntime::EngineRuntime(const PlatformServices &platform)
    : m_platform(platform) {
}

static DisplayWindowProps get_window_props_from_config(const std::string &app_name, const ApplicationConfig &config) {
    return {
        .caption = config.display.caption.empty()
                     ? app_name
                     : config.display.caption,
        .size = config.display.size,
        .min_size = std::nullopt,
        .max_size = std::nullopt,
        .api = get_default_api(),
        .resizable = config.display.resizable,
    };
}

void EngineRuntime::init(IApplicationHandler &app_handler) {
    Logger::get().add_sink(FileLogSink::open("runtime.log"));

    m_asset_service.emplace(); /* HACK: asset service must be a valid object for .value() to work */

    m_app = std::unique_ptr<IApplication>(app_handler.create_app(
        IApplication::Servers{
            .input = m_input_service,
            .asset = m_asset_service.value(),
        },
        m_platform));
    if (!m_app) {
        LOG_CRITICAL("Failed to attach application during runtime.");
        exit(-1);
    }

    ApplicationConfig config;
    m_app->configure(config);
    m_app_config = std::move(config);

    /* initialize fps counter */
    m_fps = {};

    auto &os = m_platform.os;
    LOG_INFO("platform: {}", os.get_platform());
    LOG_INFO("os: {}", os.get_os());

    /* create main window/screen */
    auto &display = m_platform.display;

    auto props = get_window_props_from_config(m_app->get_name(), m_app_config);
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
    m_asset_service.reset();
    m_asset_service.emplace(::AssetService::Provider{
        .renderer = {
            .load_texture = [](ImageData &image, void *userdata) -> TextureHandle {
                ASSERT(userdata);
                auto &renderer = *static_cast<std::unique_ptr<Renderer> *>(userdata);
                return renderer->load_texture(image);
            },
            .unload_texture = [](TextureHandle texture, void *userdata) -> void {
                ASSERT(userdata);
                auto &renderer = *static_cast<std::unique_ptr<Renderer> *>(userdata);
                renderer->destroy_texture(texture);
            },
            .userdata = &m_renderer,
        },
    });
    /* poll window size */
    Vec2i display_size = display.get_size(m_window);
    LOG_INFO("display size: {0}", display_size);
    m_input_service.init();

    /* initialize application */
    m_app->on_init();
}

bool EngineRuntime::count_fps(float delta) {
    m_fps.timer += delta;
    m_fps.accumulator++;

    /* if a second has passed,
     * transfer accumulator to fps */
    if (m_fps.timer >= 1.0f) {
        m_fps.fps = m_fps.accumulator;
        m_fps.accumulator = {};
        m_fps.timer = {};

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

    /* update caption debug info */
    if (new_second) {
        std::string title = std::format("{0} | {1}fps", m_app_config.display.caption, m_fps.fps);
        m_platform.display.set_display_caption(m_window, title);
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
    UpdateContext update_ctx{
        .frame = build_frame_context(delta),
        .new_second = new_second,
    };
    m_app->on_update(update_ctx);
}

void EngineRuntime::render() {
    m_queue.clear();

    /* build render frame! */
    RenderContext render{
        .frame = m_frame_ctx,
        .view_context = nullptr,
        .queue = m_queue,
        .clear_color = Color{25, 25, 25, 0},
    };

    m_app->on_draw(render);
    render_frame(render);
}

void EngineRuntime::render_frame(RenderContext &ctx) {
    m_renderer->clear(ctx.clear_color);
    ASSERT(ctx.view_context, "View context must be built every frame!");
    m_renderer->render_pass(*ctx.view_context, ctx.queue);
    m_renderer->swap_buffers();
}

void EngineRuntime::close() {
    m_app->on_close();
    m_app.reset();

    m_input_service.close();
    m_asset_service.reset();

    m_renderer.reset();
    m_platform.display.close_display(m_window);

    Logger::get().flush();
}

FrameContext &EngineRuntime::build_frame_context(float delta) {
    m_frame_ctx = {
        .delta = delta,
        .fps = m_fps.fps,
        .viewport = m_renderer->get_viewport(),
    };
    return m_frame_ctx;
}
