#pragma once

#include <memory>
#include <string_view>

#include "asset_cache.h"
#include "renderer/api/render_handles.h"

class ImageData;
class AssetService;

using TextureRef = AssetRef<TextureHandle>;

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
};
