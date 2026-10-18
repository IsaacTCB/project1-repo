#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "core/assertion.h"
#include "graphics/api/render_device_factory.h"
#include "runtime/debug/imgui_backend.h"

#include <SDL3/SDL.h>

template <typename FactorySpecs>
class BackendRegistry {
public:
    [[nodiscard]]
    const FactorySpecs *get(std::string_view name) const {
        for (const auto &entry : m_backends) {
            if (entry.key == name) {
                return &entry.value;
            }
        }
        return nullptr;
    }

    [[nodiscard]]
    FactorySpecs *get(std::string_view name) {
        const auto *const_this = const_cast<const BackendRegistry<FactorySpecs> *>(this);
        return const_cast<FactorySpecs *>(const_this->get(name));
    }

    void add(std::string_view name, FactorySpecs &&spec) {
        ASSERT(!get(name));
        m_backends.emplace_back(KeyTable{
            .key = std::string{name},
            .value = std::forward<FactorySpecs>(spec),
        });
    }

    [[nodiscard]]
    std::vector<std::string> list() const {
        std::vector<std::string> backends;
        backends.reserve(m_backends.size());

        for (const auto &entry : m_backends) {
            backends.emplace_back(entry.key);
        }

        return backends;
    }

private:
    /* NOTE: this has to be ordered, because we want to
     * prioritize certain backends over others. Lookup
     * becomes O(n) because of this, unfortunately. */
    struct KeyTable {
        std::string key;
        FactorySpecs value;
    };
    std::vector<KeyTable> m_backends;
};

struct SDL_FactorySpecs {
    uint32_t (*get_flags)() = nullptr;
    IRenderDeviceFactory *(*create)(SDL_Window *window) = nullptr;
    IImGuiBackend *(*get_imgui_backend)(SDL_Window *window) = nullptr;
};

BackendRegistry<SDL_FactorySpecs> get_backend_registry();
