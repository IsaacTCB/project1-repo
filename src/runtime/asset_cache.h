#pragma once

#include <functional>
#include <string_view>
#include <unordered_map>

#include "asset_ref.h"
#include "core/assertion.h"

template <typename Handle>
class AssetCache {
public:
    using LoadFn = std::function<Handle(std::string_view)>;
    using UnloadFn = std::function<void(Handle)>;

private:
    class CacheSlot;

public:
    /**
     * @brief Sets load and unload callbacks
     * for a given function.
     *
     * @param load_fn Callback used for loading. Passes std::string_view as a parameter
     * and must return a valid Handle.
     * @param unload_fn Callback used when unloading. Passes Handle as an argument.
     */
    template <typename Func1, typename Func2>
        requires std::convertible_to<Func1, LoadFn> &&
                 std::convertible_to<Func2, UnloadFn>
    void set_load_callbacks(Func1 &&load_fn, Func2 &&unload_fn) {
        m_load_fn = std::forward<Func1>(load_fn);
        m_unload_fn = std::forward<Func2>(unload_fn);
    }

    /**
     * @brief Fetches a handle from the cache. Whenever a cache miss happens,
     * due to the texture not being loaded, it calls m_load_fn to load it.
     *
     * @param path Cache key. Currently only represents path in virtual filesystem.
     * @return An AssetRef with proper ref counting support.
     */
    AssetRef<Handle> get(std::string_view path) {
        size_t hash = get_hash(path);
        CacheSlot *slot;
        if (auto it = m_cache.find(hash); it != m_cache.end()) {
            slot = &it->second;
        } else {
            ASSERT(m_load_fn, "Cache attempted to load without having load callback!");
            Handle handle = m_load_fn(path);
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

private:
    void destroy(CacheSlot *slot) {
        ASSERT(m_unload_fn, "Cache attempted to unload without having load callback!");
        this->m_unload_fn(slot->handle);
        this->m_cache.erase(slot->hash);
    }

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
    LoadFn m_load_fn;
    UnloadFn m_unload_fn;
    std::unordered_map<size_t, CacheSlot> m_cache;
};
