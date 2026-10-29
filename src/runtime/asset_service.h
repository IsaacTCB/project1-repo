#pragma once

#include <memory>
#include <string_view>

#include "core/image/image.h"
#include "core/math/vec2i.h"
#include "runtime/renderer/render_handles.h"

#include "asset_ref.h"

/* TODO: move asset definitions outta here */
struct TextureAsset {
    TextureHandle handle;
    PixelFormat format;
    Vec2i size;

    operator TextureHandle() const { return handle; }
    operator bool() const { return handle.valid(); }
};

class TextureRef : public AssetRefMixin<TextureRef, TextureAsset> {
public:
    using AssetRefMixin::AssetRefMixin;

    /* texture metadata */
    [[nodiscard]] TextureHandle handle() const { return this->get().handle; }
    [[nodiscard]] PixelFormat format() const { return this->get().format; }
    [[nodiscard]] Vec2i size() const { return this->get().size; };
    [[nodiscard]] int width() const { return this->get().size.x; };
    [[nodiscard]] int height() const { return this->get().size.y; };

    /* misc */
    [[nodiscard]] bool valid() const { return static_cast<bool>(this->get()); }
    [[nodiscard]] operator bool() const { return this->valid(); }
};

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
    [[nodiscard]] TextureAsset load_texture(std::string_view path) const;
    void unload_texture(const TextureAsset &texture) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    const Provider m_provider;
};
