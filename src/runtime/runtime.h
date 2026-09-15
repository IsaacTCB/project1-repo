#pragma once

#include <optional>

#include "renderer/renderer_2d.h"

#include "app/application_handler.h"
#include "asset_service.h"
#include "input_service.h"
#include "platform_services.h"
#include "runtime/app/application.h"
#include "runtime/app/context/frame_context.h"
#include "runtime/audio/audio_system.h"
#include "runtime/server/display_server.h"
#include "runtime/system/view/view_system_2d.h"

class EngineRuntime {
public:
    explicit EngineRuntime(const PlatformServices &platform);

    void init(IApplicationHandler &app_handler);
    void update(float delta);
    void render();
    void close();

private:
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
    bool init_display_and_renderer(const DisplayWindowProps &props);

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
    bool try_init_backend(const std::string &backend, const DisplayWindowProps &props);

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

    void render_frame(RenderContext &ctx);

private:
    PlatformServices m_platform;

private:
    FrameContext m_frame_ctx;
    DisplayWindowHandle m_window;
    std::optional<Renderer2D> m_renderer;
    RenderQueue m_queue;

    std::unique_ptr<AudioSystem> m_audio_system;
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
};
