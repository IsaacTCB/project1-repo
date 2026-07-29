#pragma once

#include <memory>
#include <optional>

#include "renderer/api/render_queue.h"
#include "renderer/api/renderer.h"

#include "app/application_handler.h"
#include "asset_service.h"
#include "input_service.h"
#include "platform_services.h"
#include "runtime/app/context/frame_context.h"

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

private:
    PlatformServices m_platform;
    IApplicationHandler *m_app_handler;

private:
    FrameContext m_frame_ctx;
    DisplayWindowHandle m_window;
    std::unique_ptr<Renderer> m_renderer;
    RenderQueue m_queue;

    InputService m_input_service;
    std::optional<AssetService> m_asset_service;
};
