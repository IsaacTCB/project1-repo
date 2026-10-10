#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "core/assertion.h"
#include "graphics/api/render_device_factory.h"
#include "runtime/debug/imgui_backend.h"

#include <SDL3/SDL.h>

template <typename FactorySpecs>
class BackendRegistry {
public:
    [[nodiscard]]
    FactorySpecs *get(const std::string &name) {
        auto it = m_backends.find(name);
        if (it == m_backends.end()) {
            return nullptr;
        }

        return &it->second;
    }

    void add(const std::string &name, FactorySpecs &&spec) {
        ASSERT(!get(name));
        m_backends[name] = std::forward<FactorySpecs>(spec);
    }

    [[nodiscard]]
    std::vector<std::string> list() const {
        std::vector<std::string> backends;
        backends.reserve(m_backends.size());

        for (const auto &[k, v] : m_backends) {
            backends.emplace_back(k);
        }

        return backends;
    }

private:
    /* NOTE: this has to be ordered, because we want to
     * prioritize certain backends over others */
    std::map<std::string, FactorySpecs> m_backends;
};

struct SDL_FactorySpecs {
    uint32_t (*get_flags)() = nullptr;
    IRenderDeviceFactory *(*create)(SDL_Window *window) = nullptr;
    IImGuiBackend *(*get_imgui_backend)(SDL_Window *window) = nullptr;
};

BackendRegistry<SDL_FactorySpecs> get_backend_registry();
