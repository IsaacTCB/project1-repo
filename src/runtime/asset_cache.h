#pragma once

#include <functional>
#include <string_view>
#include <unordered_map>

#include "asset_ref.h"

template <typename TAsset>
class AssetCache {
public:
    using LoadFn = std::function<TAsset(std::string_view)>;
    using UnloadFn = std::function<void(const TAsset &)>;

public:
    /**
     * @brief Sets load and unload callbacks
     * for a given function.
     *
     * @param load_fn Callback used for loading. Passes std::string_view as a parameter
     * and must return a valid TAsset object.
     * @param unload_fn Callback used when unloading. Passes TAsset as an argument.
     */
    template <typename Func1, typename Func2>
    void set_load_callbacks(Func1 &&load_fn, Func2 &&unload_fn);

    /**
     * @brief Fetches an asset from the cache.
     *
     * Whenever a cache miss happens, it calls m_load_fn to fetch
     * the necessary asset from disk, or whatever you need it to do.
     *
     * @param path Cache key. Currently only represents path in filesystem.
     * @return An AssetRef with proper ref counting support.
     */
    [[nodiscard]] AssetRef<TAsset> get(std::string_view path);

    [[nodiscard]] static size_t get_hash(std::string_view path);

private:
    class CacheSlot : public IAssetControlBlock<TAsset> {
    public:
        template <typename MakeAsset>
        explicit CacheSlot(AssetCache<TAsset> *cache, size_t hash, MakeAsset &&make_asset);
        static void destroy_callback(IAssetControlBlock<TAsset> *block);

    public:
        AssetCache<TAsset> *cache;
        size_t hash;
    };

    void destroy(CacheSlot *slot);

private:
    LoadFn m_load_fn;
    UnloadFn m_unload_fn;
    std::unordered_map<size_t, CacheSlot> m_cache;
};

#include "asset_cache.inl.h" // IWYU pragma: keep
