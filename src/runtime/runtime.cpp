#include "runtime.h"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "core/assertion.h"
#include "core/events/input_events.h"
#include "core/events/platform_events.h"
#include "core/events/window_events.h"
#include "core/input/keyboard.h"
#include "core/logger.h"
#include "core/math/color.h"
#include "core/math/vec2.h"
#include "core/math/vec2i.h"
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
#include "runtime/debug/sinks/file_log_sink.h"
#include "runtime/renderer/render_queue_builder.h"
#include "runtime/system/view/view_system_2d.h"
#include "runtime/version.h"
#include "server/display_server.h"
#include "server/os_server.h"

#if ENABLE_IMGUI
#include "runtime/debug/imgui_editor.h"
#endif

static const auto s_version_name = RuntimeVersion::version().full_semver_name();

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
        .api = "",
        .resizable = config.display.resizable,
    };
}

EngineRuntime::LaunchResult EngineRuntime::process_launch_args(std::span<char *const> args) {
    /* HACK: We are currently using std::printf to print some stuff. We should either be
     * using an in-house solution or std::println instead (the latter breaks ci mingw-w64
     * builds). */

    const std::string_view program = !args.empty() && args[0] ? args[0] : "app";

    const auto print_usage = [program]() -> void {
        std::printf(
            "Usage:\n"
            "  %s [options]\n"
            "\n"
            "OPTIONS\n"
            "  -h, --help             show this message and exit\n"
            "  --version              show runtime version id\n"
            "  -v, --verbose          enable detailed log levels\n"
            "  -L, --export-log       output logs to a file\n"
            "\n"
            "DISPLAY OPTIONS\n"
            "  --render-device        select what render device backend to use\n"
            "  --list-render-devices  list available render device backends\n"
            "  --max-fps              overrides the maximum framerate. Pass 0 to disable framelimiter.\n",
            program.data());
    };

    const auto print_version = []() -> void {
        std::puts(s_version_name.c_str());
    };

    const auto print_render_devices = [this]() -> void {
        const auto backends = m_platform.display.get_render_backends();
        std::puts("Supported backends:");

        for (const auto &b : backends) {
            std::printf("  '%s'\n", b.data());
        }
    };

    const auto is_render_device_available = [this](std::string_view backend) -> bool {
        const auto backends = m_platform.display.get_render_backends();
        return std::ranges::contains(backends, backend);
    };

    for (size_t i = 1; i < args.size(); ++i) {
        const std::string_view arg = args[i];
        if (arg == "-h" || arg == "--help") {
            print_usage();
            std::fflush(stdout);
            return LaunchResult::EXIT_OK;
        }
        if (arg == "--version") {
            print_version();
            std::fflush(stdout);
            return LaunchResult::EXIT_OK;
        }
        if (arg == "-v" || arg == "--verbose") {
            Logger::get().set_severity(LOG_SEVERITY_TRACE);
            if (LOG_DEFAULT_SEVERITY_LEVEL == LOG_SEVERITY_TRACE) {
                LOG_TRACE("verbose mode was already enabled. Specifying -v/--verbose is redudant.");
            } else {
                LOG_TRACE("enabled verbose mode. Trace messages will get printed");
            }
            std::fflush(stdout);
            continue;
        }
        if (arg == "--list-render-devices") {
            print_render_devices();
            std::fflush(stdout);
            return LaunchResult::EXIT_OK;
        }
        if (arg == "-L" || arg == "--export-log") {
            /* read options */
            const std::string_view opt_arg =
                (i + 1) < args.size()
                    ? args[i + 1]
                    : std::string_view{};

            /* no option was specified */
            if (opt_arg.empty()) {
                LOG_ERROR("Please specify an output path for the log file");
                std::fflush(stdout);
                return LaunchResult::EXIT_ERROR;
            }

            m_export_log = opt_arg;
            i++;

            continue;
        }
        if (arg == "--render-device") {
            /* read options */
            const std::string_view opt_arg =
                (i + 1) < args.size()
                    ? args[i + 1]
                    : std::string_view{};

            /* no option was specified */
            if (opt_arg.empty()) {
                LOG_ERROR("Please specify a render device backend.");
                print_render_devices();
                std::fflush(stdout);
                return LaunchResult::EXIT_ERROR;
            }

            if (!is_render_device_available(opt_arg)) {
                LOG_ERROR(
                    "RenderDevice '{}' does not exist or "
                    "is not supported by this platform",
                    opt_arg);
                print_render_devices();
                std::fflush(stdout);
                return LaunchResult::EXIT_ERROR;
            }

            m_preferred_backend = opt_arg;
            i++; /* skip next iteration */

            continue;
        }
        if (arg == "--max-fps") {
            /* read options */
            const std::string opt_arg =
                (i + 1) < args.size()
                    ? std::string{args[i + 1]}
                    : std::string{};

            /* no option was specified */
            if (opt_arg.empty()) {
                LOG_ERROR("Please specify the maximum fps amount (or 0 for no cap).");
                std::fflush(stdout);
                return LaunchResult::EXIT_ERROR;
            }

            long long fps_amount = 0;
            const auto [ptr, ec] = std::from_chars(opt_arg.data(), opt_arg.data() + opt_arg.size(), fps_amount);
            if (ec == std::errc::result_out_of_range) {
                LOG_ERROR("The maximum fps amount is out of range.");
                std::fflush(stdout);
                return LaunchResult::EXIT_ERROR;
            }
            if (ec == std::errc::invalid_argument) {
                LOG_ERROR("The maximum fps parameter must be a number.");
                std::fflush(stdout);
                return LaunchResult::EXIT_ERROR;
            }
            if (fps_amount < 0) {
                LOG_ERROR("The maximum fps amount cannot be a negative number.");
                std::fflush(stdout);
                return LaunchResult::EXIT_ERROR;
            }

            m_fps_override.emplace(
                static_cast<uint32_t>(fps_amount));
            i++; /* skip next iteration */

            continue;
        }

        LOG_ERROR("Unknown command '{}'", arg);
        print_usage();
        return LaunchResult::EXIT_ERROR;
    }

    /* commit name */
    m_program_name = program;

    return LaunchResult::CONTINUE;
}

void EngineRuntime::init(IApplicationHandler &app_handler) {
#if DEBUG
    m_export_log =
        m_export_log.empty()
            ? "runtime.log"
            : m_export_log;
#endif

    if (!m_export_log.empty()) {
        auto file = FileLogSink::open(m_export_log);
        if (file) {
            LOG_INFO("Exporting this session's logs to '{}'", m_export_log.generic_string());
            Logger::get().add_sink(std::move(file));
        } else {
            LOG_ERROR(
                "Could not export logs to '{}'. Please "
                "check filepath and file permissions.",
                m_export_log.generic_string());
        }
    }

#if ENABLE_IMGUI
    m_imgui_editor.install_log_sink();
#endif

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
    LOG_INFO("runtime version: {}", RuntimeVersion::version().semver_string);
    LOG_INFO("platform: {}", os.get_platform());
    LOG_INFO("os: {}", os.get_os());

    /* apply fps cap */
    m_platform.os.set_fps_max(
        m_fps_override.value_or(m_app_config.display.max_fps));

    if (auto fps = m_platform.os.get_fps_max(); fps > 0) {
        LOG_INFO("frame limiter: {}", fps);
    } else {
        LOG_INFO("frame limiter: {}", "off");
    }

    /* create main window/screen */
    const auto props = get_window_props_from_config(m_app->get_name(), m_app_config);
    if (!init_display_and_renderer(props, m_preferred_backend)) {
        exit(-1);
    }

    /* enable filtering */
    m_renderer->set_filtering(
        m_app_config.display.filter_textures);

#if ENABLE_IMGUI
    m_imgui_editor.init(m_platform.display, m_window);
#endif

    m_view.set_presentation({
        .logical_size = m_app_config.view.logical_size,
        .mode = m_app_config.view.mode,
        .aspect = m_app_config.view.aspect,
    });

    /* initialize asset service */
    m_asset_service.reset();
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
    Vec2i display_size = m_platform.display.get_size(m_window);
    LOG_INFO("display size: {0}", display_size);
    m_view.set_output_rect({.position = {0, 0}, .size = display_size});
    m_input_service.init();

    /* initialize application */
    m_app->on_init();
}

bool EngineRuntime::try_init_backend(std::string_view backend, const DisplayWindowProps &props) {
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

bool EngineRuntime::init_display_and_renderer(const DisplayWindowProps &props, std::string_view preferred_backend) {
    /* TODO: support explicit backend request */
    const auto supported_backends = m_platform.display.get_render_backends();
    if (supported_backends.empty()) {
        ASSERT(false, "EngineRuntime::init_display_and_renderer(): "
                      "Platform does not provide any graphics backend.");
        return false;
    }

    if (!preferred_backend.empty()) {
        if (try_init_backend(preferred_backend, props)) {
            LOG_INFO("Using '{}' as render device backend (user specified)", preferred_backend);
            return true;
        }

        LOG_CRITICAL("RenderDevice '{}' failed", preferred_backend);
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
        "Failed to start render device: unable to initialize "
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
        m_exit_requested = true;
        return;
    }

#if defined(DEBUG)
    /* update caption debug info */
    if (new_second) {
        const auto &render_size = m_view.get_layout().render_viewport.size;
        const std::string resolution = std::format("{0}x{1}", render_size.x, render_size.y);

        std::string title = std::format("{0} | {1}fps | {2} | {3}", m_app_config.display.caption, m_fps.fps, resolution, s_version_name);
        m_platform.display.set_display_caption(m_window, title);
    }
#endif

    for (const auto &event : m_platform.event.span<EventWindow>()) {
        if (const auto *ptr = std::get_if<EventWindowResized>(&event)) {
            LOG_TRACE("resized: {}", ptr->size);

            /* skip invalid window sizes for rendering */
            if (ptr->size.x <= 0 || ptr->size.y <= 0) {
                continue;
            }

            m_renderer->on_screen_resized(ptr->size);
            m_view.set_output_rect({.position = {0, 0}, .size = ptr->size});
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

#if ENABLE_IMGUI
    m_imgui_editor.update({
        .input = m_input_service,
        .view = m_view,
        .app_config = m_app_config,
        .renderer = m_renderer,
        .fps = m_fps.fps,
        .max_fps = m_platform.os.get_fps_max(),
    });
#endif

    /* update it */
    UpdateContext update_ctx{
        .frame = build_frame_context(delta),
        .new_second = new_second,
    };

    m_app->on_update(update_ctx);
}

void EngineRuntime::render() {
    if (m_exit_requested) {
        /* fixes ImGui assertion on app exit. */
        return;
    }

    m_queue.clear();

    /* build render frame! */
    RenderContext render{
        .frame = m_frame_ctx,
        .view_context = nullptr,
        .queue = RenderQueueBuilder{m_queue},
        .clear_color = Color{25, 25, 25, 0},
    };

    m_app->on_draw(render);
    ASSERT(render.view_context, "View context must be built every frame!");

    m_renderer->render_frame(render);

#if ENABLE_IMGUI
    m_imgui_editor.render();
#endif

    m_renderer->present();
}

void EngineRuntime::close() {
    /* this is require to address early-exit before
     * ::init() edgecases (happens whenever a launch
     * arg parser fails or smth) */
    if (!m_app) {
        return;
    }

    m_app->on_close();
    m_app.reset();

    m_input_service.close();
    m_asset_service.reset();

#if ENABLE_IMGUI
    m_imgui_editor.shutdown();
#endif

    m_renderer.reset();
    m_platform.display.close_display(m_window);

    Logger::get().flush();
}

FrameContext &EngineRuntime::build_frame_context(float delta) {
    m_frame_ctx = {
        .delta = delta,
        .fps = m_fps.fps,
        .view_layout = &m_view.get_layout(),
    };
    return m_frame_ctx;
}
