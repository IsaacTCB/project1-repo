#pragma once

#include <memory>
#include <optional>
#include <string>

#include "core/math/vec2i.h"
#include "graphics/api/render_device_factory.h"

#include "server_macros.inl"

/**
 * @brief Window creation properties
 */
struct DisplayWindowProps {
    std::string caption = "Window";
    Vec2i size{800, 600};
    std::optional<Vec2i> min_size;
    std::optional<Vec2i> max_size;
    std::string api;
    bool resizable = false;
};

// HACK: Ideally we return a WindowHandle object instead.
struct DisplayWindowHandle {
    void *raw_window_handle;
    DisplayWindowProps props;

    operator bool() const noexcept {
        return raw_window_handle != nullptr;
    }
};

/**
 * @class IDisplayServer
 * @brief Platform generic windowing abstraction layer.
 */
class IDisplayServer {
public:
    DECLARE_SERVER(IDisplayServer)

public:
    /**
     * @brief Initializes the window display
     *
     * @param props The desired window configuration to use.
     * @return A DisplayWindowHandle, that references a window inside this server.
     */
    virtual DisplayWindowHandle init_display(const DisplayWindowProps &props) = 0;

    /**
     * @brief Closes the window display handle.
     */
    virtual void close_display(DisplayWindowHandle handle) = 0;

    virtual void set_display_caption(DisplayWindowHandle handle, const std::string &caption) = 0;

    /**
     * @brief Gets the current window display size.
     * @return A tuple containing the window's width and height respectively.
     */
    [[nodiscard]]
    virtual Vec2i get_size(const DisplayWindowHandle &handle) const = 0;

    [[nodiscard]]
    virtual std::vector<std::string> get_render_backends() = 0;

    [[nodiscard]]
    virtual std::unique_ptr<IRenderDeviceFactory> get_render_factory(DisplayWindowHandle handle) = 0;
};
