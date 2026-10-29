#pragma once

#include "asset_cache.h"

#include "core/assertion.h"

/* == Asset Cache == */

template <typename T>
template <typename Func1, typename Func2>
void AssetCache<T>::set_load_callbacks(Func1 &&load_fn, Func2 &&unload_fn) {
    using LoadFn = AssetCache::LoadFn;
    using UnloadFn = AssetCache::UnloadFn;

    static_assert(std::convertible_to<Func1, LoadFn>);
    static_assert(std::convertible_to<Func2, UnloadFn>);

    m_load_fn = std::forward<Func1>(load_fn);
    m_unload_fn = std::forward<Func2>(unload_fn);
}

template <typename TAsset>
AssetRef<TAsset> AssetCache<TAsset>::get(std::string_view path) {
    size_t hash = get_hash(path);

    auto it = m_cache.find(hash);
    if (it == m_cache.end()) {
        ASSERT(m_load_fn, "Cache attempted to load without having load callback!");

        auto [emplaced, success] = m_cache.try_emplace(
            hash, /* hash key */
            /* args used in constructor */
            this,
            hash, /* hash stored in object for lookup */
            [&]() { return m_load_fn(path); });
        (void)success;

        it = emplaced;

        /* HACK: we assume that all assets contain
         * handle. that may or may not be true. */
        if (!it->second.handle) {
            m_cache.erase(it);
            return AssetRef<TAsset>{};
        }
    }

    CacheSlot *slot = &it->second;
    ++slot->ref_count;
    return AssetRef<TAsset>{*slot};
}

template <typename TAsset>
size_t AssetCache<TAsset>::get_hash(std::string_view path) {
    static std::hash<std::string_view> hash_generator{};
    return hash_generator(path);
}

template <typename TAsset>
void AssetCache<TAsset>::destroy(CacheSlot *slot) {
    ASSERT(m_unload_fn, "Cache attempted to unload without having load callback!");
    this->m_unload_fn(slot->handle);
    this->m_cache.erase(slot->hash);
}

/* == Asset Cache slot == */

template <typename TAsset>
template <typename MakeAsset>
AssetCache<TAsset>::CacheSlot::CacheSlot(AssetCache<TAsset> *cache, size_t hash, MakeAsset &&make_asset)
    : IAssetControlBlock<TAsset>(CacheSlot::destroy_callback, std::forward<MakeAsset>(make_asset)),
      cache(cache), hash(hash) {}

template <typename TAsset>
void AssetCache<TAsset>::CacheSlot::destroy_callback(IAssetControlBlock<TAsset> *block) {
    auto slot = static_cast<CacheSlot *>(block);
    slot->cache->destroy(slot);
}
