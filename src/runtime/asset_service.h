#pragma once

#include <memory>
#include <string_view>

#include "runtime/renderer/render_handles.h"

#include "asset_ref.h"

using TextureRef = AssetRef<TextureHandle>;

class ImageData;
class AssetService {
public:
    class Provider {
    public:
        using LoadFn = TextureHandle (*)(ImageData &image, void *userdata);
        using UnloadFn = void (*)(TextureHandle handle, void *userdata);

        struct {
            LoadFn load_texture = nullptr;
            UnloadFn unload_texture = nullptr;
            void *userdata = nullptr;
        } renderer{};
    };

public:
    explicit AssetService();
    explicit AssetService(const ::AssetService::Provider &provider);
    ~AssetService();

    /* TODO: implement functionality for this. */
    void clear();

    TextureRef get_texture(std::string_view path);

private:
    [[nodiscard]] TextureHandle load_texture(std::string_view path) const;
    void unload_texture(TextureHandle handle) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    const Provider m_provider;
};
