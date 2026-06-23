#pragma once

#include "runtime/platform_services.h"

#include <memory>

class IApplication {
public:
    virtual ~IApplication() = default;

public:
    virtual void on_init() = 0;
    virtual void on_update(float delta) = 0;
    virtual void on_render() = 0;
    virtual void on_close() = 0;

protected:
    friend class ApplicationHandler;
    static std::unique_ptr<IApplication> create_application(const PlatformServices &platform);

    explicit IApplication(const PlatformServices *platform) : m_platform{*platform} {};
    PlatformServices m_platform; // non-owning reference to platform.
};

#define DECLARE_APP(AppName) \
public:                      \
    AppName(const PlatformServices *platform) : IApplication(platform) {}

#define SET_APP(AppName)                                                                               \
    std::unique_ptr<IApplication> IApplication::create_application(const PlatformServices &platform) { \
        return std::unique_ptr<IApplication>(new AppName(&platform));                                  \
    }
#define APPLICATION_DEFINED
