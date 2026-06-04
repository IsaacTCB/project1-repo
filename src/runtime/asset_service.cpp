#include "asset_service.h"
#include "renderer/api/render_handles.h"

#include <atomic>
#include <unordered_map>
#include <utility>

#include "core/image/image.h"
#include "core/logger.h"
#include "runtime/image/decoder.h"

struct Slot {
    Slot(AssetService *asset_service, size_t hash, TextureHandle handle)
        : asset_service(asset_service), hash(hash), handle(handle), ref_count(0) {
    }

    void inc_ref() {
        LOG_TRACE("inc ref {}", ref_count.load());
        ref_count++;
    }

    void dec_ref() {
        if (ref_count == 0 || --ref_count == 0) {
            asset_service->remove_texture(this);
        }
    }

    AssetService *asset_service;
    size_t hash{};
    TextureHandle handle;
    std::atomic_size_t ref_count;
};

struct AssetService::Impl {
    std::unordered_map<size_t, Slot> textures;
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

    Slot *slot = nullptr;
    if (auto it = m_impl->textures.find(hash); it != m_impl->textures.end()) {
        slot = &it->second;
    } else {
        auto [it2, success] = m_impl->textures.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(hash),
            std::forward_as_tuple(this, hash, load_texture(path)));
        ASSERT(success, "unordered map emplace failed");

        slot = &it2->second;
    }

    slot->ref_count++;
    return TextureRef{slot};
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

void AssetService::remove_texture(void *slot) {
    auto *ptr = static_cast<Slot *>(slot);
    unload_texture(ptr->handle);
    m_impl->textures.erase(ptr->hash);
}

TextureRef::~TextureRef() {
    release();
}

void TextureRef::reset() {
    *this = {};
}

void TextureRef::acquire() {
    if (m_ptr) {
        static_cast<Slot *>(m_ptr)->inc_ref();
    }
}

void TextureRef::release() {
    if (m_ptr) {
        auto *slot = static_cast<Slot *>(m_ptr);
        m_ptr = nullptr;
        slot->dec_ref();
    }
}

TextureHandle TextureRef::get() const {
    return static_cast<Slot *>(m_ptr)->handle;
}

TextureRef::TextureRef(const TextureRef &other)
    : m_ptr(other.m_ptr) {
    acquire();
}

TextureRef::TextureRef(TextureRef &&other) noexcept
    : m_ptr(other.m_ptr) {
    other.m_ptr = nullptr;
}

TextureRef &TextureRef::operator=(const TextureRef &rhs) {
    if (this == &rhs) {
        return *this;
    }

    release();
    m_ptr = rhs.m_ptr;
    acquire();
    return *this;
}

TextureRef &TextureRef::operator=(TextureRef &&rhs) noexcept {
    if (this == &rhs) {
        return *this;
    }

    release();
    m_ptr = rhs.m_ptr;
    rhs.m_ptr = nullptr;
    return *this;
}
