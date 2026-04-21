#pragma once

#include "server_macros.inl"

#include "renderer/api/renderer_apis.h"

#include <cstdint>
#include <string>

/**
 * @brief Window creation properties
 */
struct DisplayWindowProps {
    std::string caption = "Window";
    uint32_t width = 800;
    uint32_t height = 600;
    RendererAPI api = get_default_api();
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
 * @class DisplayServer
 * @brief Platform generic windowing abstraction layer.
 */
class DisplayServer {
    DECLARE_SERVER(DisplayServer);

public:
    /**
     * @brief Initializes the window display
     *
     * @param props The desired window configuration to use.
     * @return A DisplayWindowHandle, that references a window inside this server.
     */
    DisplayWindowHandle init_display(const DisplayWindowProps &props);

    /**
     * @brief Closes the window display handle.
     */
    void close_display(DisplayWindowHandle handle);

    /**
     * @brief Gets the current window display size.
     * @return A tuple containing the window's width and height respectively.
     */
    [[nodiscard]]
    std::tuple<uint32_t, uint32_t> get_size(const DisplayWindowHandle &handle) const;
};

#undef DECLARE_SERVER
