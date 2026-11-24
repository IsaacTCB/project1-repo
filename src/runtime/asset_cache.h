#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "asset_ref.h"

struct TransparentStringHash {
    using is_transparent = void;
    [[nodiscard]] std::size_t operator()(std::string_view sv) const noexcept {
        return std::hash<std::string_view>{}(sv);
    }
    [[nodiscard]] std::size_t operator()(const std::string &s) const noexcept {
        return std::hash<std::string_view>{}(s);
    }
    [[nodiscard]] std::size_t operator()(const char *s) const noexcept {
        return std::hash<std::string_view>{}(s);
    }
};

struct TransparentStringEqual {
    using is_transparent = void;
    [[nodiscard]] constexpr bool operator()(std::string_view a, std::string_view b) const noexcept {
        return a == b;
    }
};

template <
    typename TAsset,
    typename TKey = std::string,
    typename THash = TransparentStringHash,
    typename TKeyEqual = TransparentStringEqual>
class AssetCache {
public:
    using AssetCacheType = AssetCache<TAsset, TKey, THash, TKeyEqual>;
    using KeyType = TKey;
    using Hasher = THash;
    using KeyEqual = TKeyEqual;

    using LoadFn = std::function<TAsset(const TKey &)>;
    using UnloadFn = std::function<void(const TAsset &)>;

    explicit AssetCache(THash hasher = THash{}, TKeyEqual key_equal = TKeyEqual{});

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
    template <typename T>
    [[nodiscard]] AssetRef<TAsset> get(T &&key);

    template <typename T>
    [[nodiscard]] size_t get_hash(T &&path) const;

private:
    class CacheSlot : public IAssetControlBlock<TAsset> {
    public:
        template <typename MakeAsset>
        CacheSlot(
            AssetCache *cache,
            TKey key,
            std::size_t hash,
            MakeAsset &&make_asset);

        static void destroy_callback(IAssetControlBlock<TAsset> *block);

    public:
        AssetCacheType *cache;
        TKey key;
        size_t hash = 0;
    };

    void destroy(CacheSlot *slot);

private:
    LoadFn m_load_fn;
    UnloadFn m_unload_fn;
    [[no_unique_address]] THash m_hasher;
    [[no_unique_address]] TKeyEqual m_key_equal;
    std::unordered_map<TKey, CacheSlot, THash, TKeyEqual> m_cache;
};

#include "asset_cache.inl.h" // IWYU pragma: keep
