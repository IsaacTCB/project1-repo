#include "asset_service.h"

#include <filesystem>
#include <fstream>
#include <string_view>
#include <utility>

#include "core/image/image.h"
#include "core/logger.h"

#include "runtime/asset_cache.h"
#include "runtime/asset_ref.h"
#include "runtime/fonts/decoders/font_truetype.h"
#include "runtime/image/decoder.h"
#include "runtime/renderer/render_handles.h"

namespace {

template <typename T>
void hash_combine(std::size_t &seed, const T &value) {
    seed ^= std::hash<T>{}(value) +
            (seed << 6) +
            (seed >> 2);
}

struct FontCacheKey {
    std::string path;
    float pixel_height; /* <--- dunno if it's a good idea to store floats as key but oh well */

    bool operator==(const FontCacheKey &other) const {
        return path == other.path &&
               pixel_height == other.pixel_height;
    }
};

struct FontCacheKeyHash {
    std::size_t operator()(const FontCacheKey &key) const {
        std::size_t seed = 0;
        hash_combine(seed, key.path);
        hash_combine(seed, key.pixel_height);
        return seed;
    }
};

} // namespace

struct AssetService::Impl {
    AssetCache<TextureAsset> textures;
    AssetCache<FontResource, FontCacheKey, FontCacheKeyHash, std::equal_to<>> fonts;
    FontLoader font_loader;
};

AssetService::AssetService() = default;

AssetService::AssetService(const Provider &provider)
    : m_provider{provider} {
    m_impl = std::make_unique<Impl>();

    /* setup font loader */
    m_impl->font_loader.set_callbacks({
        .callback = [](std::vector<uint8_t> bitmap, int width, int height, void *userdata) {
            auto *asset = static_cast<AssetService *>(userdata);

            /* HACK: renderer does not support grayscale images (yet!), so
             * convert 1 channel grayscale image into 4 channel rgba */
            std::vector<uint8_t> converted;
            converted.resize(width * height * 4);
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    const uint8_t coverage = bitmap[x + (y * width)];

                    uint8_t *dst = &converted[(x + (y * width)) * 4];
                    dst[2] = dst[1] = dst[0] = 0xFF;
                    dst[3] = coverage;
                }
            }

            ImageData image{
                .width = static_cast<size_t>(width),
                .height = static_cast<size_t>(height),
                .format = PixelFormat::RGBA8,
                .pixels = std::move(converted),
            };
            return asset->m_provider.renderer.load_texture(image, asset->m_provider.renderer.userdata)
                .handle;
        },
        .userdata = this,
    });

    /* config texture callbacks */
    auto texture_load_fn = [this](const std::string &path) {
        return this->load_texture(path);
    };
    auto texture_unload_fn = [this](const auto &handle) {
        this->unload_texture(handle);
    };
    m_impl->textures.set_load_callbacks(texture_load_fn, texture_unload_fn);

    /* config font callbacks */
    auto font_load_fn = [this](const FontCacheKey &font) {
        return this->load_font(font.path, font.pixel_height);
    };
    auto font_unload_fn = [this](const FontResource &handle) {
        m_provider.renderer.unload_texture(
            handle.atlas, m_provider.renderer.userdata);
    };
    m_impl->fonts.set_load_callbacks(font_load_fn, font_unload_fn);
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

namespace {

std::optional<std::vector<uint8_t>> load_file(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> file_data(size);
    file.read(
        reinterpret_cast<char *>(file_data.data()),
        file_data.size());

    return file_data;
}

}; // namespace

FontRef AssetService::get_font(std::string_view path, float pixel_height) {
    return m_impl->fonts.get(FontCacheKey{
        .path = std::string{path},
        .pixel_height = pixel_height,
    });
}

FontResource AssetService::load_font(std::string_view path, float pixel_height) {
    auto file = load_file(path);
    if (!file) {
        return {};
    }
    const auto font = m_impl->font_loader.load_truetype(std::move(*file),
        {
            .pixel_height = pixel_height,
            .oversampling = 2,
        });

    if (!font) {
        return {};
    }

    LOG_TRACE("loaded font '{}' ph={}", path, pixel_height);
    return *font;
}
