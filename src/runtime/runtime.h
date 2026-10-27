#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>

#include "core/debug/profiler/profiler.h"
#include "renderer/renderer_2d.h"

#include "app/application_handler.h"
#include "asset_service.h"
#include "input_service.h"
#include "platform_services.h"
#include "runtime/app/application.h"
#include "runtime/app/context/frame_context.h"
#include "runtime/server/display_server.h"
#include "runtime/system/view/view_system_2d.h"

#if ENABLE_IMGUI
#include "debug/imgui_profiler.h"
#endif

class EngineRuntime {
public:
    enum class LaunchResult : uint8_t {
        CONTINUE,
        EXIT_OK,
        EXIT_ERROR,
    };

public:
    explicit EngineRuntime(const PlatformServices &platform);

    LaunchResult process_launch_args(std::span<char *const> args);

    void init(IApplicationHandler &app_handler);
    void update(float delta);
    void render();
    void close();

private:
    void begin_frame();
    void end_frame();

    /**
     * @brief Initializes the main window and renderer.
     *
     * This function runs through the supported graphics
     * backend from the platform until one works.
     *
     * @note props.api is currently ignored.
     *
     * @param props The display props for instantiating
     * the window. `props.api` is ignored.
     * @return True if the display and renderer were
     * initialized successfully, false otherwise
     */
    bool init_display_and_renderer(const DisplayWindowProps &props, std::string_view backend = {});

    /**
     * @brief Tries to initialize window + renderer from the
     * display server.
     *
     * Used by init_display_and_renderer.
     *
     * @param backend Specifies what render device backend to use.
     * @param props The display props for instantiating
     * the window. `props.api` gets overwritten by backend.
     * @return True if the operation was successful, false otherwise.
     */
    bool try_init_backend(std::string_view backend, const DisplayWindowProps &props);

    /**
     * @brief Builds a new frame context and updates
     * m_frame_ctx.
     *
     * @param delta Elapsed time since last frame
     * @return An updated reference of m_frame_ctx
     */
    FrameContext &build_frame_context(float delta);

    /**
     * @brief Updates fps counter
     *
     * @param delta Elapsed time since last frame
     * @return If this is the first frame for this second
     */
    bool count_fps(float delta);

#if ENABLE_IMGUI
    void imgui_editor();
    void imgui_console();
#endif

private:
    PlatformServices m_platform;

private:
    FrameContext m_frame_ctx;
    DisplayWindowHandle m_window;
    std::optional<Renderer2D> m_renderer;
    RenderQueue m_queue;

#if ENABLE_IMGUI
    std::unique_ptr<IImGuiBackend> m_imgui;
    std::shared_ptr<class ImGuiLogState> m_imgui_log_state;
    ImGuiProfiler m_imgui_profiler;
#endif

    std::string m_program_name;
    std::filesystem::path m_export_log{};
    std::string m_preferred_backend;
    ProfilerFrame m_profiler_frame{};

    ViewSystem m_view;

    struct FpsCounter {
        uint32_t fps;
        uint32_t accumulator;
        float timer;
    } m_fps{};

    InputService m_input_service;
    std::optional<AssetService> m_asset_service;

    std::unique_ptr<IApplication> m_app;
    ApplicationConfig m_app_config;

    bool m_exit_requested = false;
};
