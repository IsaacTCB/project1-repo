#pragma once

#include <optional>

#include "renderer/renderer.h"

#include "app/application_handler.h"
#include "asset_service.h"
#include "input_service.h"
#include "platform_services.h"
#include "runtime/app/context/frame_context.h"
#include "runtime/server/display_server.h"

class EngineRuntime {
public:
    explicit EngineRuntime(const PlatformServices &platform);

    void init(IApplicationHandler &app_handler);
    void update(float delta);
    void render();
    void close();

private:
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
    IApplicationHandler *m_app_handler;

private:
    FrameContext m_frame_ctx;
    DisplayWindowHandle m_window;
    std::optional<Renderer> m_renderer;
    RenderQueue m_queue;

    uint32_t m_fps, m_fps_accum;
    float m_fps_timer;

    InputService m_input_service;
    std::optional<AssetService> m_asset_service;
};
