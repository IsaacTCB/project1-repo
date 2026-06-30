#pragma once

#include <memory>

#include "renderer/api/render_queue.h"
#include "runtime/app/application.h"
#include "runtime/platform_services.h"

class IApplicationHandler {
protected:
    virtual IApplication *create_app(
        const IApplication::Servers &app_servers,
        const PlatformServices &platform_service) = 0;

public:
    void init(const IApplication::Servers &app_servers, const PlatformServices &platform_services);
    void update(float delta);
    void render(RenderQueue &queue);
    void close();

private:
    std::unique_ptr<IApplication> m_app;
};
