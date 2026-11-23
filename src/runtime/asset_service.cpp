#include "asset_service.h"

#include <string_view>
#include <utility>

#include "core/image/image.h"
#include "core/logger.h"

#include "runtime/asset_cache.h"
#include "runtime/asset_ref.h"
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

namespace {

/**
 * @brief An exclusive asset service control block used to
 * handle procedurally generated assets at runtime ref counting.
 * E.g. a gradient texture that was created on the fly at runtime.
 *
 * @tparam TAsset The asset data itself
 * @tparam Allocator A struct containing at least two static functions: make(AssetService*, ...) and destroy(AssetService*, TAsset&).
 */
template <typename TAsset, typename Allocator>
class ProceduralAssetBlock : public IAssetControlBlock<TAsset> {
public:
    /* TODO: use C++ 20 concepts to force a compile-time contract */

    template <typename... Args>
    static AssetRef<TAsset> create(AssetService &asset, Args &&...args) {
        auto *block = new ProceduralAssetBlock(&asset, std::forward<Args>(args)...);
        if (!block) {
            /* allocation failed, just quit */
            return AssetRef<TAsset>{};
        }

        if (!block->handle) {
            /* if the resulting asset is invalid, then
             * we deallocate the control block */
            delete block;
            return AssetRef<TAsset>{};
        }
        return AssetRefBuilder::create<TAsset>(*block);
    }

private:
    template <typename... Args>
    explicit ProceduralAssetBlock(AssetService *asset, Args &&...args)
        : IAssetControlBlock<TAsset>(
              ProceduralAssetBlock::destroy_callback,
              [&]() { return Allocator::make(asset, std::forward<Args>(args)...); }),
          m_asset(asset) {
    }

    static void destroy_callback(IAssetControlBlock<TAsset> *caller) {
        auto *block = static_cast<ProceduralAssetBlock *>(caller);
        Allocator::destroy(block->m_asset, block->handle);
        delete caller;
    }
    AssetService *m_asset;
};

} // namespace

TextureRef AssetService::create_texture(ImageData &image) {
    ASSERT(image, "attempted to create invalid texture");
    ASSERT(m_provider.renderer.load_texture);

    struct TextureAssetAllocator {
        static TextureAsset make(AssetService *asset, ImageData &image) {
            return asset->m_provider
                .renderer.load_texture(
                    image,
                    asset->m_provider.renderer.userdata);
        }
        static void destroy(AssetService *asset, TextureAsset &texture) {
            asset->unload_texture(texture);
        }
    };
    return ProceduralAssetBlock<TextureAsset, TextureAssetAllocator>::create(*this, image);
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
