#include "asset_service.h"

#include <string_view>

#include "core/image/image.h"
#include "core/logger.h"
#include "renderer/api/render_handles.h"
#include "runtime/asset_cache.h"
#include "runtime/image/decoder.h"

struct AssetService::Impl {
    AssetCache<TextureHandle> textures;
};

AssetService::AssetService(const Provider &provider)
    : m_provider{provider} {
    m_impl = std::make_unique<Impl>();

    /* config texture callbacks */
    auto texture_load_fn = [this](std::string_view path) {
        return this->load_texture(path);
    };
    auto texture_unload_fn = [this](auto handle) {
        this->unload_texture(handle);
    };
    m_impl->textures.set_load_callbacks(texture_load_fn, texture_unload_fn);
}

AssetService::~AssetService() {
    clear();
}

/* Getters  */

void AssetService::clear() {
    /* TODO: invalidate every asset */
}

TextureRef AssetService::get_texture(std::string_view path) {
    return m_impl->textures.get(path);
}

/* Loaders */

TextureHandle AssetService::load_texture(std::string_view path) const {
    ASSERT(m_provider.renderer.load_texture);

    ImageData image = Image::decode(path);
    if (!image) {
        LOG_CRITICAL("No '{}' image file found in the current working directory.", path);
        exit(-1);
    }

    LOG_TRACE("loading texture {}", path);
    return m_provider.renderer.load_texture(image, m_provider.renderer.userdata);
}

void AssetService::unload_texture(TextureHandle handle) const {
    LOG_TRACE("unloading texture {}", handle);
    ASSERT(m_provider.renderer.unload_texture);
    m_provider.renderer.unload_texture(handle, m_provider.renderer.userdata);
}
