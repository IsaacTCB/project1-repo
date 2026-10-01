#include "runtime.h"

#include <algorithm>
#include <memory>
#include <optional>

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
#include "runtime/system/view/view_system_2d.h"
#include "server/display_server.h"
#include "server/os_server.h"

#if ENABLE_IMGUI
#include "runtime/debug/imgui_backend.h"
#include "runtime/debug/imgui_style.h"
#include <imgui.h>
#include <imgui_internal.h>

/**
 * @brief Gets the dockspace's central node rectangle as a viewport.
 *
 * @return The dockspace's available viewport, or std::nullopt if the dockspace has no central node.
 */
static std::optional<Viewport> get_dockspace_viewport(ImGuiID dockspace_id) {
    const ImGuiDockNode *central = ImGui::DockBuilderGetCentralNode(dockspace_id);
    if (!central) {
        return std::nullopt;
    }

    const auto origin = ImGui::GetMainViewport()->Pos;
    Vec2i min = {
        static_cast<int>(central->Pos.x - origin.x),
        static_cast<int>(central->Pos.y - origin.y),
    };
    Vec2i max = {
        static_cast<int>(central->Pos.x + central->Size.x - origin.x),
        static_cast<int>(central->Pos.y + central->Size.y - origin.y),
    };

    const Vec2i size = max - min;
    if (size.x <= 0 || size.y <= 0) {
        return std::nullopt;
    }

    return Viewport{
        .position = min,
        .size = size,
    };
}
#endif

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

void EngineRuntime::init(IApplicationHandler &app_handler) {
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
    m_app_config = config;

    /* initialize fps counter */
    m_fps = {};

    auto &os = m_platform.os;
    LOG_INFO("platform: {}", os.get_platform());
    LOG_INFO("os: {}", os.get_os());

    /* create main window/screen */
    const auto props = get_window_props_from_config(m_app->get_name(), m_app_config);
    if (!init_display_and_renderer(props)) {
        exit(-1);
    }

    /* enable filtering */
    m_renderer->set_filtering(
        m_app_config.display.filter_textures);

#if ENABLE_IMGUI
    /* open ImGui context and configure it */
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGuiCustomStyles::apply_dark_mode(ImGui::GetStyle());

    /* open ImGui backend */
    m_imgui = m_platform.display.create_imgui_backend(m_window);
    if (m_imgui && !m_imgui->init()) {
        LOG_WARN("ImGui backend unavailable, ImGui output will be discarded");

        /* ImGui backend failed, fallback to null backend */
        m_imgui.reset(imgui_create_null_backend());
        ASSERT(m_imgui);
        bool result = m_imgui->init();
        ASSERT(result, "Failed to initialize fallback ImGui NULL backend");
    }
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
        m_exit_requested = true;
        return;
    }

#if defined(DEBUG)
    /* update caption debug info */
    if (new_second) {
        const auto &render_size = m_view.get_layout().render_viewport.size;
        const std::string resolution = std::format("{0}x{1}", render_size.x, render_size.y);

        std::string title = std::format("{0} | {1}fps | {2} | Debug", m_app_config.display.caption, m_fps.fps, resolution);
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
    if (m_imgui) {
        m_imgui->new_frame();
        ImGui::NewFrame();

        ImGuiID dockspace_id = ImGui::GetID("MyMainDockspace");
        ImGui::DockSpaceOverViewport(dockspace_id, ImGui::GetMainViewport(),
            ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingOverCentralNode);

        if (const auto dockspace_viewport = get_dockspace_viewport(dockspace_id)) {
            m_view.set_output_rect(*dockspace_viewport);
        }

        imgui_editor();
    }
#endif

    /* update it */
    UpdateContext update_ctx{
        .frame = build_frame_context(delta),
        .new_second = new_second,
    };

    m_app->on_update(update_ctx);
}

#if ENABLE_IMGUI
void EngineRuntime::imgui_editor() {
    /* TODO: temporary code. Move it into its own special code. */
    static bool s_menu_bar = true;
    if (m_input_service.is_key_just_pressed(Input::Key::Scancode::F1)) {
        s_menu_bar = !s_menu_bar;
        LOG_TRACE("{} ImGui...", s_menu_bar ? "enabling" : "disabling");
    }

    static bool s_demo_open = false;
    if (s_menu_bar) {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Debug")) {
                if (ImGui::MenuItem("Demo Window")) {
                    s_demo_open = true;
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        if (ImGui::Begin("Details & Settings")) {
            ImGui::TextWrapped("Adjust settings");

            /* ================== *
             *   Detail settings   *
             * ================== */
            ImGui::SeparatorText("Details");

            const auto &layout = m_view.get_layout();
            ImGui::Text("fps = %d", m_fps.fps);
            ImGui::Text("render_device = %s", "Not implemented!");

            /* ================= *
             *   View settings   *
             * ================= */
            ImGui::SeparatorText("View");

            /* Logical View size */
            bool updated_view = false;
            if (ImGui::DragFloat2("View Size", &m_app_config.view.logical_size.x, 1.0f, 1.0f, 10000.0f, "%.1f")) {
                updated_view = true;
            }

            /* Filtering */
            static const char *filtering_items[] = {
                "Nearest",
                "Bilinear",
            };
            static const bool filtering_lookup[] = {false, true};
            int filtering_mode = static_cast<int>(m_app_config.display.filter_textures);
            if (ImGui::Combo("Texture Filtering", &filtering_mode, filtering_items, IM_ARRAYSIZE(filtering_items))) {
                m_app_config.display.filter_textures = filtering_lookup[filtering_mode];
                m_renderer->set_filtering(m_app_config.display.filter_textures);
            }

            /* PresentationMode */
            static const char *presentation_policy_items[] = {
                "Disabled",
                "Fixed Resolution",
                "Logical View",
            };
            static const PresentationMode presentation_policy_lookup[] = {
                PresentationMode::DISABLED,
                PresentationMode::FIXED_RESOLUTION,
                PresentationMode::LOGICAL_VIEW,
            };
            int presentation_mode = static_cast<int>(m_app_config.view.mode);
            if (ImGui::Combo("Presentation Mode", &presentation_mode, presentation_policy_items, IM_ARRAYSIZE(presentation_policy_items))) {
                m_app_config.view.mode = presentation_policy_lookup[presentation_mode];
                updated_view = true;
            }

            /* AspectPolicy */
            static const char *aspect_policy_items[] = {
                "Ignore",
                "Keep Width",
                "Keep Height",
                "Expand",
            };
            static const AspectPolicy aspect_policy_lookup[] = {
                AspectPolicy::IGNORE,
                AspectPolicy::KEEP_WIDTH,
                AspectPolicy::KEEP_HEIGHT,
                AspectPolicy::EXPAND,
            };
            int aspect_mode = static_cast<int>(m_app_config.view.aspect);
            if (ImGui::Combo("Aspect Mode", &aspect_mode, aspect_policy_items, IM_ARRAYSIZE(aspect_policy_items))) {
                m_app_config.view.aspect = aspect_policy_lookup[aspect_mode];
                updated_view = true;
            }
            ImGui::Text("logical_size = %.0fx%.0f", layout.logical_size.x, layout.logical_size.y);
            ImGui::Text("render_target_size = %dx%d", layout.render_target_size.x, layout.render_target_size.y);

            if (updated_view) {
                LOG_TRACE("updated view");
                m_view.set_presentation({
                    .logical_size = m_app_config.view.logical_size,
                    .mode = m_app_config.view.mode,
                    .aspect = m_app_config.view.aspect,
                });
            }
        }
        ImGui::End();

        if (s_demo_open) {
            ImGui::ShowDemoWindow(&s_demo_open);
        }
    }
}
#endif

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
        .queue = m_queue,
        .clear_color = Color{25, 25, 25, 0},
    };

    m_app->on_draw(render);
    ASSERT(render.view_context, "View context must be built every frame!");

    m_renderer->render_frame(render);

#if ENABLE_IMGUI
    /* build ImGui data and render it */
    if (m_imgui) {
        ImGui::Render();
        m_imgui->render();
    }
#endif

    m_renderer->present();
}

void EngineRuntime::close() {
    m_app->on_close();
    m_app.reset();

    m_input_service.close();
    m_asset_service.reset();

#if ENABLE_IMGUI
    /* close ImGui backend & context */
    if (m_imgui) {
        m_imgui->shutdown();
        m_imgui.reset();
    }
    ImGui::DestroyContext();
#endif

    m_renderer.reset();
    m_platform.display.close_display(m_window);
}

FrameContext &EngineRuntime::build_frame_context(float delta) {
    m_frame_ctx = {
        .delta = delta,
        .fps = m_fps.fps,
        .view_layout = &m_view.get_layout(),
    };
    return m_frame_ctx;
}
