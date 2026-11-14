#include "asset_service.h"

#include <string_view>

#include "core/image/image.h"
#include "core/logger.h"

#include "runtime/asset_cache.h"
#include "runtime/image/decoder.h"
#include "runtime/renderer/render_handles.h"

struct AssetService::Impl {
    AssetCache<TextureAsset> textures;
};

AssetService::AssetService() = default;

AssetService::AssetService(const Provider &provider)
    : m_provider{provider} {
    m_impl = std::make_unique<Impl>();

    /* config texture callbacks */
    auto texture_load_fn = [this](std::string_view path) {
        return this->load_texture(path);
    };
    auto texture_unload_fn = [this](const auto &handle) {
        this->unload_texture(handle);
    };
    m_impl->textures.set_load_callbacks(texture_load_fn, texture_unload_fn);
}

AssetService::~AssetService() {
    clear();
}

void AssetService::clear() {
    /* TODO: invalidate every asset */
}

/* Getters  */

TextureRef AssetService::get_texture(std::string_view path) {
    return m_impl->textures.get(path);
}

/* Loaders */

TextureAsset AssetService::load_texture(std::string_view path) const {
    ASSERT(m_provider.renderer.load_texture);

    ImageData image = Image::decode(path);
    if (!image) {
        LOG_WARN("Unable to load image at \"{}\"", path);
        return TextureAsset{};
    }

    LOG_TRACE("loading texture {}", path);
    return m_provider.renderer.load_texture(image, m_provider.renderer.userdata);
}

void AssetService::unload_texture(const TextureAsset &texture) const {
    LOG_TRACE("unloading texture {}", texture.handle);
    ASSERT(m_provider.renderer.unload_texture);
    ASSERT(texture.handle, "Attempted to destroy an invalid handle");
    m_provider.renderer.unload_texture(texture.handle, m_provider.renderer.userdata);
}
