#pragma once

#include <memory>

#include "runtime/app/application.h"
#include "runtime/app/context/frame_context.h"
#include "runtime/platform_services.h"

class IApplicationHandler {
protected:
    virtual IApplication *create_app(
        const IApplication::Servers &app_servers,
        const PlatformServices &platform_service) = 0;

public:
    void init(const IApplication::Servers &app_servers, const PlatformServices &platform_services);
    void update(UpdateContext &ctx);
    void render(RenderContext &ctx);
    void close();

private:
    std::unique_ptr<IApplication> m_app;
};
