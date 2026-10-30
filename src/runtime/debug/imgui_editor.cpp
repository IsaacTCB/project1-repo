#include "imgui_editor.h"

#include <cstddef>
#include <optional>

#include <imgui.h>
#include <imgui_internal.h>

#include "core/input/keyboard.h"
#include "core/logger.h"
#include "core/math/vec2i.h"
#include "runtime/debug/imgui_style.h"
#include "runtime/debug/sinks/imgui_log_sink.h"

#include "core/assertion.h"

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

ImGuiEditor::ImGuiEditor() = default;
ImGuiEditor::~ImGuiEditor() = default;

void ImGuiEditor::install_log_sink() {
    m_log_state = std::make_shared<ImGuiLogState>(ImGuiLogState::DEFAULT_LINE_COUNT);
    Logger::get().add_sink(ImGuiLogSink::create(m_log_state));
}

void ImGuiEditor::init(IDisplayServer &display, DisplayWindowHandle window) {
    /* open ImGui context and configure it */
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGuiCustomStyles::apply_dark_mode(ImGui::GetStyle());

    /* open ImGui backend */
    m_backend = display.create_imgui_backend(window);
    if (m_backend && !m_backend->init()) {
        LOG_WARN("ImGui backend unavailable, ImGui output will be discarded");

        /* ImGui backend failed, fallback to null backend */
        m_backend.reset(imgui_create_null_backend());
        ASSERT(m_backend);
        bool result = m_backend->init();
        ASSERT(result, "Failed to initialize fallback ImGui NULL backend");
    }
}

void ImGuiEditor::update(const Context &ctx) {
    if (!m_backend) {
        return;
    }

    m_backend->new_frame();
    ImGui::NewFrame();

    ImGuiID dockspace_id = ImGui::GetID("MyMainDockspace");
    ImGui::DockSpaceOverViewport(dockspace_id, ImGui::GetMainViewport(),
        ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingOverCentralNode);

    if (const auto dockspace_viewport = get_dockspace_viewport(dockspace_id)) {
        ctx.view.set_output_rect(*dockspace_viewport);
    }

    draw_editor(ctx);
}

void ImGuiEditor::render() {
    /* build ImGui data and render it */
    if (m_backend) {
        ImGui::Render();
        m_backend->render();
    }
}

void ImGuiEditor::shutdown() {
    /* close ImGui backend & context */
    if (m_backend) {
        m_backend->shutdown();
        m_backend.reset();
    }
    ImGui::DestroyContext();
}

void ImGuiEditor::draw_editor(const Context &ctx) {
    if (ctx.input.is_key_just_pressed(Input::Key::Scancode::F1)) {
        m_menu_bar = !m_menu_bar;
        LOG_TRACE("{} ImGui...", m_menu_bar ? "enabling" : "disabling");
    }

    if (m_menu_bar) {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Debug")) {
                if (ImGui::MenuItem("Demo Window")) {
                    m_demo_open = true;
                }
                ImGui::MenuItem("Console", nullptr, &m_console_open);
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

            const auto &layout = ctx.view.get_layout();
            ImGui::Text("fps = %d", ctx.fps);

            const IRenderDevice *device = ctx.renderer ? ctx.renderer->device() : nullptr;
            ImGui::Text("render_device = %s", device ? device->name() : "n/a");

            /* ================= *
             *   View settings   *
             * ================= */
            ImGui::SeparatorText("View");

            /* Logical View size */
            bool updated_view = false;
            if (ImGui::DragFloat2("View Size", &ctx.app_config.view.logical_size.x, 1.0f, 1.0f, 10000.0f, "%.1f")) {
                updated_view = true;
            }

            /* Filtering */
            static const char *filtering_items[] = {
                "Nearest",
                "Bilinear",
            };
            static const bool filtering_lookup[] = {false, true};
            int filtering_mode = static_cast<int>(ctx.app_config.display.filter_textures);
            if (ImGui::Combo("Texture Filtering", &filtering_mode, filtering_items, IM_ARRAYSIZE(filtering_items))) {
                ctx.app_config.display.filter_textures = filtering_lookup[filtering_mode];
                ctx.renderer->set_filtering(ctx.app_config.display.filter_textures);
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
            int presentation_mode = static_cast<int>(ctx.app_config.view.mode);
            if (ImGui::Combo("Presentation Mode", &presentation_mode, presentation_policy_items, IM_ARRAYSIZE(presentation_policy_items))) {
                ctx.app_config.view.mode = presentation_policy_lookup[presentation_mode];
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
            int aspect_mode = static_cast<int>(ctx.app_config.view.aspect);
            if (ImGui::Combo("Aspect Mode", &aspect_mode, aspect_policy_items, IM_ARRAYSIZE(aspect_policy_items))) {
                ctx.app_config.view.aspect = aspect_policy_lookup[aspect_mode];
                updated_view = true;
            }
            ImGui::Text("logical_size = %.0fx%.0f", layout.logical_size.x, layout.logical_size.y);
            ImGui::Text("render_target_size = %dx%d", layout.render_target_size.x, layout.render_target_size.y);

            if (updated_view) {
                LOG_TRACE("updated view");
                ctx.view.set_presentation({
                    .logical_size = ctx.app_config.view.logical_size,
                    .mode = ctx.app_config.view.mode,
                    .aspect = ctx.app_config.view.aspect,
                });
            }
        }
        ImGui::End();

        if (m_demo_open) {
            ImGui::ShowDemoWindow(&m_demo_open);
        }

        if (m_console_open) {
            draw_console();
        }
    }
}

void ImGuiEditor::draw_console() {
    ImGui::Begin("Console");

    static ImGuiTextFilter s_console_filter;
    static bool s_autoscroll = true;
    static size_t s_generation = 0;

    static const char *s_severity_items[] = {
        "Trace",
        "Info",
        "Warn",
        "Error",
        "Critical",
    };
    static int s_severity_selected = LOG_SEVERITY_TRACE;

    ImGui::Checkbox("Auto-scroll", &s_autoscroll);

    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        m_log_state->clear();
    }

    ImGui::SameLine();
    ImGui::PushItemWidth(80.0f);
    ImGui::Combo("##console_log_severity", &s_severity_selected, s_severity_items, IM_ARRAYSIZE(s_severity_items));
    ImGui::PopItemWidth();

    ImGui::SameLine();
    s_console_filter.Draw("##console_log_filter", ImGui::GetContentRegionAvail().x);

    ImGui::BeginChild("##console_log", ImVec2(0, 0), 0, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 2.0f);

    m_log_state->for_each([](const ImGuiLogState::Line &line) {
        const char *beg = line.string.c_str();
        const char *end = beg + line.string.size();

        if (line.severity < static_cast<LogSeverity>(s_severity_selected)) {
            return;
        }

        if (!s_console_filter.PassFilter(beg, end)) {
            return;
        }

        switch (line.severity) {
            case LOG_SEVERITY_TRACE:
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.55f, 0.525f, 1.0f));
                break;
            case LOG_SEVERITY_WARNING:
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.98f, 0.8f, 0.05f, 1.0f));
                break;
            case LOG_SEVERITY_ERROR:
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.07f, 0.05f, 1.0f));
                break;
            case LOG_SEVERITY_CRITICAL:
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.98f, 0.96f, 1.0f));
                break;
            case LOG_SEVERITY_INFO:
            default:
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.98f, 0.96f, 1.0f));
                break;
        }

        if (line.severity == LOG_SEVERITY_CRITICAL) {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 size = ImGui::CalcTextSize(line.string.c_str());

            ImGui::GetWindowDrawList()->AddRectFilled(
                pos,
                ImVec2(pos.x + size.x, pos.y + size.y),
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.82f, 0.06f, 0.04f, 0.95f)));
        }

        ImGui::TextUnformatted(beg, end);

        ImGui::PopStyleColor();
    });

    size_t gen = m_log_state->generation();
    if (s_generation != gen) {
        if (s_autoscroll) {
            ImGui::SetScrollHereY(1.0f);
        }
        s_generation = gen;
    }

    ImGui::PopStyleVar();

    ImGui::EndChild();
    ImGui::End();
}
