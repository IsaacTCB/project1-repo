#pragma once

#include <functional>
#include <string_view>
#include <unordered_map>

#include "asset_ref.h"
#include "core/assertion.h"
#include "core/handles.h"

template <typename T>
class AssetCache {
private:
    class CacheSlot : public IAssetControlBlock<T> {
    public:
        explicit CacheSlot(AssetCache<Handle> *cache, size_t hash, Handle handle)
            : IAssetControlBlock<Handle>(handle), cache(cache), hash(hash) {}

        void destroy() override {
            cache->destroy(this);
        };

    public:
        AssetCache<T> *cache;
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
    AssetRef<T> get(std::string_view path) {
        size_t hash = get_hash(path);
        CacheSlot *slot;
        if (auto it = m_cache.find(hash); it != m_cache.end()) {
            slot = &it->second;
        } else {
            T handle = load_fn(path);
            auto [emplaced, success] = m_cache.try_emplace(hash, this, hash, handle);

            ASSERT(success, "Failed to emplace asset slot");
            slot = success ? &emplaced->second
                           : nullptr;
        }

        if (!slot) {
            /* in case that we weren't
             * able to fetch it */
            return AssetRef<T>{};
        }

        slot->ref_count++;
        return AssetRef<T>{*slot};
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
    std::unordered_map<size_t, CacheSlot> m_cache;
};
