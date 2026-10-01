#pragma once

#include "core/math/vec2i.h"
#include "runtime/app/context/frame_context.h"
#include "runtime/asset_service.h"
#include "runtime/audio/audio_system.h"
#include "runtime/input_service.h"
#include "runtime/platform_services.h"
#include "runtime/system/view/view_system_2d.h"

#include <memory>
#include <string>

struct ApplicationConfig {
    struct Display {
        std::string caption = "Untitled";
        Vec2i size = {800, 600};
        bool resizable = false;
        bool filter_textures = true;

    } display;

    struct View {
        Vec2 logical_size = {800.0f, 600.0f};
        PresentationMode mode = PresentationMode::DISABLED;
        AspectPolicy aspect = AspectPolicy::KEEP_HEIGHT;

    } view;
};

class IApplication {
public:
    struct Servers {
        InputService &input;
        AssetService &asset;
        std::unique_ptr<AudioSystem> &audio;
    };

public:
    virtual ~IApplication() = default;

    /**
     * @brief Returns the name of the current application
     *
     * @return A const char *pointer to a null-terminated
     * string of the name.
     */
    [[nodiscard]]
    virtual const char *get_name() const = 0;

public:
    /**
     * @brief Provides initial configuration for the engine runtime.
     *
     * This is used to setup initial window size, caption and other
     * properties at runtime.
     *
     * Called before on_init, so don't rely on this function
     * for providing extra functionality.
     *
     * @param config A reference for an application config object
     * to update.
     */
    virtual void configure(ApplicationConfig &config) = 0;

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
    virtual void on_update(UpdateContext &ctx) = 0;

    /**
     * @brief Draws the world. Called on every frame
     * right after `IApplication::on_update()`.
     *
     * @param queue The render command list. Contains
     * methods for drawing stuff in 2D space.
     */
    virtual void on_draw(RenderContext &ctx) = 0;

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
     * See ::SET_APP(app_name) to know how to implement
     * this.
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
        : IApplication(servers, platform) {}  \
                                              \
    const char *get_name() const override {   \
        return #AppName;                      \
    }

#define SET_APP(AppName)                            \
    IApplication *IApplication::create_application( \
        const IApplication::Servers &servers,       \
        const PlatformServices &platform) {         \
        return new AppName(servers, &platform);     \
    }
