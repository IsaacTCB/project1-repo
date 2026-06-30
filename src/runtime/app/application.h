#pragma once

#include "renderer/api/render_queue.h"
#include "runtime/asset_service.h"
#include "runtime/input_service.h"
#include "runtime/platform_services.h"

class IApplication {
public:
    struct Servers {
        InputService &input;
        AssetService &asset;
    };

public:
    virtual ~IApplication() = default;

public:
    /**
     * @brief Initialization code. Called on init.
     */
    virtual void on_init() = 0;

    /**
     * @brief Update logic code. Called on every frame.
     *
     * @param delta The elapsed time in seconds between
     * this frame and the previous one.
     */
    virtual void on_update(float delta) = 0;

    /**
     * @brief Draws the world. Called on every frame
     * right after `IApplication::on_update()`.
     *
     * @param queue The render command list. Contains
     * methods for drawing stuff in 2D space.
     */
    virtual void on_draw(RenderQueue &queue) = 0;

    /**
     * @brief Deinitialization code. Called right before the
     * Application's destructor.
     */
    virtual void on_close() = 0;

protected:
    friend class ApplicationHandler;

    /**
     * @brief Application factory function. Has to
     * be implemented in the application. The caller
     * WILL have to manage its lifetime.
     *
     * See ::SET_APP(app_name) for usage
     *
     * @param servers A valid set of application
     * servers.
     * @param platform A valid set of platform
     * servers.
     *
     * @return A pointer to the newly allocated
     * application.
     */
    static IApplication *create_application(
        const Servers &servers,
        const PlatformServices &platform);

    explicit IApplication(
        const Servers &servers,
        const PlatformServices *platform)
        : m_servers{servers}, m_platform{*platform} {};

    Servers m_servers;
    PlatformServices m_platform;
};

#define DECLARE_APP(AppName)                  \
public:                                       \
    AppName(                                  \
        const IApplication::Servers &servers, \
        const PlatformServices *platform)     \
        : IApplication(servers, platform) {}

#define SET_APP(AppName)                            \
    IApplication *IApplication::create_application( \
        const IApplication::Servers &servers,       \
        const PlatformServices &platform) {         \
        return new AppName(servers, &platform);     \
    }
