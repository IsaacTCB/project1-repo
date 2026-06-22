#pragma once

#include "asset_cache.h"

#include "core/assertion.h"

/* == Asset Cache == */

template <typename T>
template <typename Func1, typename Func2>
void AssetCache<T>::set_load_callbacks(Func1 &&load_fn, Func2 &&unload_fn) {
    using LoadFn = typename AssetCache::LoadFn;
    using UnloadFn = typename AssetCache::UnloadFn;

    static_assert(std::convertible_to<Func1, LoadFn>);
    static_assert(std::convertible_to<Func2, UnloadFn>);

    m_load_fn = std::forward<Func1>(load_fn);
    m_unload_fn = std::forward<Func2>(unload_fn);
}

template <typename Handle>
AssetRef<Handle> AssetCache<Handle>::get(std::string_view path) {
    size_t hash = get_hash(path);
    CacheSlot *slot = nullptr;
    if (auto it = m_cache.find(hash); it != m_cache.end()) {
        slot = &it->second;
    } else {
        ASSERT(m_load_fn, "Cache attempted to load without having load callback!");
        Handle handle = m_load_fn(path);
        if (handle) {
            slot = emplace_slot(hash, handle);
        }
    }

    if (!slot) {
        return AssetRef<Handle>{};
    }

    slot->ref_count++;
    return AssetRef<Handle>{*slot};
}

template <typename Handle>
size_t AssetCache<Handle>::get_hash(std::string_view path) {
    static std::hash<std::string_view> hash_generator{};
    return hash_generator(path);
}

template <typename Handle>
void AssetCache<Handle>::destroy(CacheSlot *slot) {
    ASSERT(m_unload_fn, "Cache attempted to unload without having load callback!");
    this->m_unload_fn(slot->handle);
    this->m_cache.erase(slot->hash);
}

/* == Asset Cache slot == */

template <typename Handle>
AssetCache<Handle>::CacheSlot::CacheSlot(AssetCache<Handle> *cache, size_t hash, Handle handle)
    : IAssetControlBlock<Handle>(CacheSlot::destroy_callback, handle),
      cache(cache), hash(hash) {}

template <typename Handle>
AssetCache<Handle>::CacheSlot *AssetCache<Handle>::emplace_slot(size_t hash, Handle handle) {
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

template <typename Handle>
void AssetCache<Handle>::CacheSlot::destroy_callback(IAssetControlBlock<Handle> *block) {
    auto slot = static_cast<CacheSlot *>(block);
    slot->cache->destroy(slot);
}
