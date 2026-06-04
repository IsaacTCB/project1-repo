#include "asset_service.h"
#include "renderer/api/render_handles.h"

#include <unordered_map>

#include "core/image/image.h"
#include "core/logger.h"
#include "runtime/image/decoder.h"

struct AssetService::Impl {
    std::unordered_map<size_t, TextureHandle> textures;
};

AssetService::AssetService(const Provider &provider)
    : m_provider{provider} {
    m_impl = std::make_unique<Impl>();
}

AssetService::~AssetService() {
    clear();
}

/* Getters  */

void AssetService::clear() {
    /* TODO: invalidate every asset */
}

TextureRef AssetService::get_texture(std::string_view path) {
    std::hash<std::string_view> hash_generator{};
    size_t hash = hash_generator(path);

    TextureRef ref{this, 0};
    if (auto it = m_impl->textures.find(hash); it != m_impl->textures.end()) {
        ref.handle = it->second;
    } else {
        ref.handle = load_texture(path);
        m_impl->textures[hash] = ref.handle;
    }

    return ref;
}

/* Loaders */

TextureHandle AssetService::load_texture(std::string_view path) const {
    ASSERT(m_provider.renderer.load_texture);

    ImageData image = Image::decode(path);
    if (!image) {
        LOG_CRITICAL("No '{}' image file found in the current working directory.", path);
        exit(-1);
    }

    return m_provider.renderer.load_texture(image, m_provider.renderer.userdata);
}

void AssetService::unload_texture(TextureHandle handle) const {
    ASSERT(m_provider.renderer.unload_texture);
    m_provider.renderer.unload_texture(handle, m_provider.renderer.userdata);
}
