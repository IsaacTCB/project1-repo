#pragma once

#include <memory>

#include "runtime/app/application.h"
#include "runtime/platform_services.h"

class IApplicationHandler {
public:
    virtual IApplication *create_app(
        const IApplication::Servers &app_servers,
        const PlatformServices &platform_service) = 0;

private:
    std::unique_ptr<IApplication> m_app;
};
