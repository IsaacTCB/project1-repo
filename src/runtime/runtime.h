#pragma once

#include <memory>
#include <optional>

#include "renderer/api/render_queue.h"
#include "renderer/api/renderer.h"

#include "app/application_handler.h"
#include "asset_service.h"
#include "input_service.h"
#include "platform_services.h"

class EngineRuntime {
public:
    explicit EngineRuntime(const PlatformServices &platform);

    void init(IApplicationHandler &app_handler);
    void update(float delta);
    void render();
    void close();

private:
    PlatformServices m_platform;
    IApplicationHandler *m_app_handler;

private:
    DisplayWindowHandle m_window;
    std::unique_ptr<Renderer> m_renderer;
    RenderQueue m_queue;

    InputService m_input_service;
    std::optional<AssetService> m_asset_service;
};
