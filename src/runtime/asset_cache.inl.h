#pragma once

#include "asset_cache.h"

#include "core/assertion.h"

/* == Asset Cache == */

template <typename TAsset, typename TKey, typename THash, typename TKeyEqual>
AssetCache<TAsset, TKey, THash, TKeyEqual>::AssetCache(THash hasher, TKeyEqual key_equal)
    : m_hasher(std::move(hasher)), m_key_equal(std::move(key_equal)), m_cache(0, m_hasher, m_key_equal) {}

template <typename TAsset, typename TKey, typename THash, typename TKeyEqual>
template <typename Func1, typename Func2>
void AssetCache<TAsset, TKey, THash, TKeyEqual>::set_load_callbacks(Func1 &&load_fn, Func2 &&unload_fn) {
    using LoadFn = AssetCache::LoadFn;
    using UnloadFn = AssetCache::UnloadFn;

    static_assert(std::convertible_to<Func1, LoadFn>);
    static_assert(std::convertible_to<Func2, UnloadFn>);

    m_load_fn = std::forward<Func1>(load_fn);
    m_unload_fn = std::forward<Func2>(unload_fn);
}

template <typename TAsset, typename TKey, typename THash, typename TKeyEqual>
template <typename T>
AssetRef<TAsset> AssetCache<TAsset, TKey, THash, TKeyEqual>::get(T &&key) {
    auto it = m_cache.find(key);
    if (it == m_cache.end()) {
        ASSERT(m_load_fn, "Cache attempted to load without having load callback!");

        const size_t hash = get_hash(std::forward<T>(key));
        if constexpr (std::is_same_v<std::decay_t<T>, TKey>) {
            auto [emplaced, success] = m_cache.try_emplace(
                key,
                this,
                key,
                hash,
                [&]() { return m_load_fn(key); });
            (void)success;
            it = emplaced;
        } else {
            TKey owned_key(key);
            auto [emplaced, success] = m_cache.try_emplace(
                owned_key,
                this,
                owned_key,
                hash,
                [&]() { return m_load_fn(owned_key); });
            (void)success;
            it = emplaced;
        }

        /* HACK: we assume that all assets contain
         * handle. that may or may not be true. */
        if (!it->second.handle) {
            m_cache.erase(it);
            return AssetRef<TAsset>{};
        }
    }

    CacheSlot *slot = &it->second;
    ++slot->ref_count;
    return AssetRefBuilder::create<TAsset>(*slot);
}

template <typename TAsset, typename TKey, typename THash, typename TKeyEqual>
template <typename T>
size_t AssetCache<TAsset, TKey, THash, TKeyEqual>::get_hash(T &&path) const {
    return m_hasher(path);
}

template <typename TAsset, typename TKey, typename THash, typename TKeyEqual>
void AssetCache<TAsset, TKey, THash, TKeyEqual>::destroy(CacheSlot *slot) {
    ASSERT(m_unload_fn, "Cache attempted to unload without having load callback!");
    auto it = m_cache.find(slot->key);
    if (it != m_cache.end() && &it->second == slot) {
        if (m_unload_fn) {
            m_unload_fn(it->second.handle);
        }
        m_cache.erase(it);
    }
}

/* == Asset Cache slot == */

template <typename TAsset, typename TKey, typename THash, typename TKeyEqual>
template <typename MakeAsset>
AssetCache<TAsset, TKey, THash, TKeyEqual>::CacheSlot::CacheSlot(AssetCache<TAsset, TKey, THash, TKeyEqual> *cache, TKey key, size_t hash, MakeAsset &&make_asset)
    : IAssetControlBlock<TAsset>(CacheSlot::destroy_callback, std::forward<MakeAsset>(make_asset)),
      cache(cache), key(key), hash(hash) {}

template <typename TAsset, typename TKey, typename THash, typename TKeyEqual>
void AssetCache<TAsset, TKey, THash, TKeyEqual>::CacheSlot::destroy_callback(IAssetControlBlock<TAsset> *block) {
    auto slot = static_cast<CacheSlot *>(block);
    slot->cache->destroy(slot);
}
