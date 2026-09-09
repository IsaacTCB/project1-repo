#pragma once

#include <memory>
#include <string>

#include "core/logger.h"
#include "graphics/api/render_device_factory.h"
#include "runtime/renderer/renderer_2d.h"
#include "runtime/server/display_server.h"

#include "sdl3_registry.h"

#include <SDL3/SDL.h>

class SDL3_DisplayServer final : public IDisplayServer {
public:
    SDL3_DisplayServer() {
        m_registry = get_backend_registry();
    }

private:
    DisplayWindowHandle init_display(const DisplayWindowProps &props) override {
        auto *specs = m_registry.get(props.api);
        if (!specs) {
            return {};
        }

        SDL_WindowFlags flags = get_common_sdl_flags(props);
        flags |= specs->get_flags ? specs->get_flags() : 0u;

        SDL_Window *window = SDL_CreateWindow(props.caption.data(), props.size.x, props.size.y, flags);
        if (!window) {
            LOG_ERROR("SDL3: Failed to create window. Reason: {0}", SDL_GetError());
            return {};
        }

        if (!configure_window(window, props)) {
            LOG_ERROR("SDL3: Failed to configure window. Reason: {0}", SDL_GetError());
            return {};
        }

        return {
            .raw_window_handle = reinterpret_cast<void *>(window),
            .props = props,
        };
    }

    void close_display(DisplayWindowHandle handle) override {
        SDL_DestroyWindow((SDL_Window *)handle.raw_window_handle);
        handle.raw_window_handle = nullptr;
    }

    [[nodiscard]]
    Vec2i get_size(const DisplayWindowHandle &handle) const override {
        auto *window = static_cast<SDL_Window *>(handle.raw_window_handle);

        Vec2i size{};
        SDL_GetWindowSize(window, &size.x, &size.y);
        return size;
    }

    [[nodiscard]]
    std::vector<std::string> get_render_backends() override {
        return m_registry.list();
    }

    [[nodiscard]]
    std::unique_ptr<IRenderDeviceFactory> get_render_factory(DisplayWindowHandle handle) override {
        auto *spec = m_registry.get(handle.props.api);
        auto *factory = spec ? spec->create(static_cast<SDL_Window *>(handle.raw_window_handle)) : nullptr;
        return std::unique_ptr<IRenderDeviceFactory>{factory};
    }

private:
    /**
     * @brief Converts a WindowProps object into native SDL window bitfield set, excluding graphical API bits
     * @param props A WindowProps objects to convert
     * @return SDL_WindowFlags object based on windows properties.
     */
    static SDL_WindowFlags get_common_sdl_flags(const DisplayWindowProps &props) {
        SDL_WindowFlags flags{};
        if (props.resizable) {
            flags |= SDL_WINDOW_RESIZABLE;
        }
        return flags;
    }

    static bool configure_window(SDL_Window *window, const DisplayWindowProps &props) {
        ASSERT(window);

        if (props.min_size) {
            SDL_SetWindowMinimumSize(window, props.min_size->x, props.min_size->y);
        }

        if (props.max_size) {
            SDL_SetWindowMaximumSize(window, props.max_size->x, props.max_size->y);
        }

        return true;
    }

    BackendRegistry<SDL_FactorySpecs> m_registry;
};
