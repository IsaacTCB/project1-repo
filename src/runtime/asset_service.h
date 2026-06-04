#pragma once

#include <string_view>

#include "core/logger.h"
#include "renderer/api/render_handles.h"

class ImageData;
class AssetService;

class TextureRef {
public:
    TextureRef() = default;
    ~TextureRef();

    TextureRef(const TextureRef &other);
    TextureRef(TextureRef &&other) noexcept;
    TextureRef &operator=(const TextureRef &rhs);
    TextureRef &operator=(TextureRef &&rhs) noexcept;

private:
    friend AssetService;
    AssetService *m_asset_service;

public:
    TextureHandle handle;

protected:
    explicit TextureRef(AssetService *service, Handle handle)
        : m_asset_service(service), handle(handle) {}
};

class AssetService {
public:
    class Provider {
    public:
        using LoadFn = TextureHandle (*)(ImageData &image, void *userdata);
        using UnloadFn = void (*)(TextureHandle handle, void *userdata);

        struct {
            LoadFn load_texture;
            UnloadFn unload_texture;
            void *userdata;
        } renderer;
    };

public:
    explicit AssetService() = delete;
    explicit AssetService(const ::AssetService::Provider &provider);
    ~AssetService();

    void clear();
    TextureRef get_texture(std::string_view path);

public:
    [[nodiscard]] TextureHandle load_texture(std::string_view path) const;
    void unload_texture(TextureHandle handle) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    const Provider m_provider;

private:
    void inc_ref(TextureRef *texture) {
        LOG_TRACE("inc ref {}", texture->handle);
    };
    void dec_ref(TextureRef *texture) {
        LOG_TRACE("dec ref {}", texture->handle);
    };

    friend TextureRef;
};

inline TextureRef::~TextureRef() {
    if (m_asset_service) {
        m_asset_service->dec_ref(this);
    }
}

inline TextureRef::TextureRef(const TextureRef &other)
    : m_asset_service(other.m_asset_service), handle(other.handle) {
    if (m_asset_service) {
        m_asset_service->inc_ref(this);
    }
}

inline TextureRef::TextureRef(TextureRef &&other) noexcept
    : m_asset_service(other.m_asset_service), handle(other.handle) {
    other.m_asset_service = nullptr;
}

inline TextureRef &TextureRef::operator=(const TextureRef &rhs) {
    this->m_asset_service = rhs.m_asset_service;
    this->handle = rhs.handle;

    if (m_asset_service) {
        m_asset_service->inc_ref(this);
    }
    return *this;
}

inline TextureRef &TextureRef::operator=(TextureRef &&rhs) noexcept {
    this->m_asset_service = rhs.m_asset_service;
    this->handle = rhs.handle;

    rhs.m_asset_service = nullptr;

    return *this;
}

// AssetService::Provider m_asset_provider{
//    .renderer = m_renderer->get_provider(),
//    .audio = m_renderer->get_provider(),
// }
//
// AssetService m_asset_service{provider};
// TextureRef texture = m_asset_service.get_texture("hello.png");
// m_asset_service.get_audio("audio.sfx");
//
