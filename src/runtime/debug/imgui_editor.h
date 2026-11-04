#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include "runtime/app/application.h"
#include "runtime/debug/imgui_backend.h"
#include "runtime/input_service.h"
#include "runtime/renderer/renderer_2d.h"
#include "runtime/server/display_server.h"
#include "runtime/system/view/view_system_2d.h"

class ImGuiLogState;

/**
 * @brief Owns the ImGui context/backend and draws the runtime's debug editor.
 */
class ImGuiEditor {
public:
    /**
     * @brief Runtime state the editor reads from and modifies.
     */
    struct Context {
        InputService &input;
        ViewSystem &view;
        ApplicationConfig &app_config;
        std::optional<Renderer2D> &renderer;
        uint32_t fps;
        uint32_t max_fps;
    };

public:
    ImGuiEditor();
    ~ImGuiEditor();

    ImGuiEditor(const ImGuiEditor &) = delete;
    ImGuiEditor &operator=(const ImGuiEditor &) = delete;

    /**
     * @brief Injects imgui console log sink, required for the imgui console
     */
    void install_log_sink();

    void init(IDisplayServer &display, DisplayWindowHandle window);
    void update(const Context &ctx);
    void render();
    void shutdown();

private:
    void draw_details(const Context &ctx);
    void draw_editor(const Context &ctx);
    void draw_console();

private:
    std::unique_ptr<IImGuiBackend> m_backend;
    std::shared_ptr<ImGuiLogState> m_log_state;
    bool m_menu_bar = true;
    bool m_demo_open = false;
    bool m_details_open = false;
    bool m_console_open = false;
};
