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

#include "renderer/render_handles.h"
#include "renderer/render_queue.h"
#include "renderer/renderer_2d.h"

#include "app/application_handler.h"
#include "asset_service.h"
#include "input_service.h"
#include "platform_services.h"
#include "runtime/app/application.h"
#include "runtime/app/context/frame_context.h"
#include "runtime/view/view_system_2d.h"
#include "server/display_server.h"
#include "server/os_server.h"

EngineRuntime::EngineRuntime(const PlatformServices &platform)
    : m_platform(platform) {
}

void EngineRuntime::init(IApplicationHandler &app_handler) {
    /* bind with app handler */
    m_app_handler = &app_handler;

    /* initialize fps counter */
    m_fps = {};

    auto &os = m_platform.os;
    LOG_INFO("platform: {}", os.get_platform());
    LOG_INFO("os: {}", os.get_os());

    constexpr Vec2i DEFAULT_RESOLUTION = {800, 600};

    /* create main window/screen */
    DisplayWindowProps props{};
    props.size = DEFAULT_RESOLUTION;
    props.resizable = true;

    m_view.set_presentation({
        .logical_size = Vec2{DEFAULT_RESOLUTION},
        .mode = PresentationMode::LOGICAL_VIEW,
        .aspect = AspectPolicy::KEEP_HEIGHT,
    });

    if (!init_display_and_renderer(props)) {
        exit(-1);
    }

    /* initialize asset service */
    m_asset_service.emplace(::AssetService::Provider{
        .renderer = {
            .load_texture = [](ImageData &image, void *userdata) -> TextureHandle {
                ASSERT(userdata);
                auto *renderer = static_cast<Renderer2D *>(userdata);
                return renderer->load_texture(image);
            },
            .unload_texture = [](TextureHandle texture, void *userdata) -> void {
                ASSERT(userdata);
                auto *renderer = static_cast<Renderer2D *>(userdata);
                renderer->destroy_texture(texture);
            },
            .userdata = &(*m_renderer),
        },
    });
    /* poll window size */
    auto &display = m_platform.display;
    Vec2i display_size = display.get_size(m_window);
    LOG_INFO("display size: {0}", display_size);
    m_view.set_output_size(display_size);
    m_input_service.init();

    /* initialize application */
    IApplication::Servers app_servers{
        .input = m_input_service,
        .asset = m_asset_service.value(),
        .view = m_view,
    };
    m_app_handler->init(app_servers, m_platform);
}

bool EngineRuntime::try_init_backend(const std::string &backend, const DisplayWindowProps &props) {
    DisplayWindowProps backend_props = props;
    backend_props.api = backend;

    auto window = m_platform.display.init_display(backend_props);
    if (!window) {
        LOG_TRACE(
            "EngineRuntime::init_display_and_renderer(): "
            "Failed to open main window with backend '{}'",
            backend);
        return false;
    }

    /* get render device factory, attach it into renderer to initialize it */
    std::unique_ptr<IRenderDeviceFactory> factory = m_platform.display.get_render_factory(window);
    if (!factory) {
        LOG_TRACE(
            "EngineRuntime::init_display_and_renderer(): Failed"
            "to obtain a valid render device factory from '{}'",
            backend);
        m_platform.display.close_display(window);
        return false;
    }

    /* init renderer */
    Renderer2D renderer{};
    if (!renderer.attach_device(std::move(factory))) {
        LOG_TRACE(
            "EngineRuntime::init_display_and_renderer(): "
            "Failed to start graphics backend '{}'",
            backend);
        m_platform.display.close_display(window);
        return false;
    }
    renderer.init();

    /* commit window and renderer */
    m_renderer = std::move(renderer);
    m_window = std::move(window);

    return true;
}

bool EngineRuntime::init_display_and_renderer(const DisplayWindowProps &props) {
    /* TODO: support explicit backend request */
    const auto supported_backends = m_platform.display.get_render_backends();
    if (supported_backends.empty()) {
        ASSERT(false, "EngineRuntime::init_display_and_renderer(): "
                      "Platform does not provide any graphics backend.");
        return false;
    }

    for (size_t i = 0; i < supported_backends.size(); ++i) {
        const auto &backend = supported_backends[i];

        if (i > 0) {
            const auto &previous_backend = supported_backends[i - 1];
            LOG_WARN(
                "RenderDevice backend '{}' failed, falling back to '{}'...",
                previous_backend,
                backend);
        }

        if (try_init_backend(backend, props)) {
            LOG_INFO("Using '{}' as render device backend", backend);
            return true;
        }
    }

    if (m_renderer.has_value()) {
        m_renderer.reset();
    }

    LOG_CRITICAL(
        "Failed to start render device: unable to initialize"
        "any of the available graphics backend.");

    return false;
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

    for (const auto &event : m_platform.event.span<EventWindow>()) {
        if (const auto *ptr = std::get_if<EventWindowResized>(&event)) {
            LOG_TRACE("resized: {}", ptr->size);

            /* skip invalid window sizes for rendering */
            if (ptr->size.x <= 0 || ptr->size.y <= 0) {
                continue;
            }

            m_renderer->on_screen_resized(ptr->size);
            m_view.set_output_size(ptr->size);
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
    m_app_handler->update(update_ctx);
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

    m_app_handler->render(render);
    render_frame(render);
}

void EngineRuntime::render_frame(RenderContext &ctx) {
    ASSERT(ctx.view_context, "View context must be built every frame!");
    m_renderer->render_frame(ctx);
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
        .fps = m_fps.fps,
        .viewport = m_renderer->get_viewport(),
    };
    return m_frame_ctx;
}
