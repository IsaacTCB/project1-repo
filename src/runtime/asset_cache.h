#pragma once

#include <functional>
#include <string_view>
#include <unordered_map>

#include "asset_ref.h"
#include "core/assertion.h"

template <typename Handle>
class AssetCache {
private:
    class CacheSlot : public IAssetControlBlock<Handle> {
    public:
        explicit CacheSlot(AssetCache<Handle> *cache, size_t hash, Handle handle)
            : IAssetControlBlock<Handle>(CacheSlot::destroy_callback, handle),
              cache(cache), hash(hash) {}

        static void destroy_callback(IAssetControlBlock<Handle> *block) {
            auto slot = static_cast<CacheSlot *>(block);
            slot->cache->destroy(slot);
        }

    public:
        AssetCache<Handle> *cache;
        size_t hash;
    };

private:
    void destroy(CacheSlot *slot) {
        if (this->unload_fn) {
            this->unload_fn(slot->handle);
        }
        this->m_cache.erase(slot->hash);
    }

public:
    AssetRef<Handle> get(std::string_view path) {
        size_t hash = get_hash(path);
        CacheSlot *slot;
        if (auto it = m_cache.find(hash); it != m_cache.end()) {
            slot = &it->second;
        } else {
            Handle handle = load_fn(path);
            /* TODO: somehow verify if handle was valid */
            slot = emplace_slot(hash, handle);
            ASSERT(slot, "Failed to emplace asset slot");
        }

        if (!slot) {
            /* in case that we weren't
             * able to fetch it */
            return AssetRef<Handle>{};
        }

        slot->ref_count++;
        return AssetRef<Handle>{*slot};
    }

    static size_t get_hash(std::string_view path) {
        static std::hash<std::string_view> hash_generator{};
        return hash_generator(path);
    }

    using LoadFn = std::function<Handle(std::string_view)>;
    LoadFn load_fn;

    using UnloadFn = std::function<void(Handle)>;
    UnloadFn unload_fn;

private:
    CacheSlot *emplace_slot(size_t hash, Handle handle) {
        auto [emplaced, success] = m_cache.try_emplace(
            hash, /* hash key */
            /* args used in constructor */
            this,
            hash, /* hash stored in object for lookup */
            handle);

        /* should always return success, but just in case... */
        return success ? &emplaced->second
                       : nullptr;
    }

private:
    std::unordered_map<size_t, CacheSlot> m_cache;
};
