#pragma once

#include <atomic>
#include <cstddef>

/**
 * @brief Controls AssetRef's ref counter, similar to
 * shared_ptr but it's intrusive.
 * An implementation class is used inside AssetCache.
 *
 * @tparam TAsset The underlying asset/handle.
 */
template <typename TAsset>
class IAssetControlBlock {
public:
    using DestroyFn = void (*)(IAssetControlBlock *caller);
    DestroyFn destroy_fn;

protected:
    explicit IAssetControlBlock();
    template <typename MakeAsset>
    explicit IAssetControlBlock(DestroyFn destroy_fn, MakeAsset &&make_asset);

public:
    void inc_ref();
    void dec_ref();
    TAsset get_handle();

public:
    std::atomic<size_t> ref_count;
    TAsset handle;
};

/* forward declaration */
template <typename TAsset>
class AssetCache;

/**
 * @brief A shared resource reference that holds an
 * asset TAsset.
 *
 * @note Users are not meant to construct this object
 * directly, only rather via a wrapper service like
 * AssetCache.
 *
 * On the surface, it behaves similarly to std::shared_ptr,
 * but internally it's a little bit more specialized and
 * thus **should** result in better performance.
 *
 * @tparam T TAsset type
 */
template <typename TAsset>
class AssetRef {
private:
    friend class AssetCache<TAsset>;
    explicit AssetRef(IAssetControlBlock<TAsset> &slot);

public:
    explicit AssetRef();
    ~AssetRef();

    AssetRef(const AssetRef &other);
    AssetRef(AssetRef &&other) noexcept;
    AssetRef &operator=(const AssetRef<TAsset> &rhs);
    AssetRef &operator=(AssetRef<TAsset> &&rhs) noexcept;

    void reset();
    void acquire();
    void release();
    [[nodiscard]] const TAsset &get() const;

private:
    IAssetControlBlock<TAsset> *m_ctrl_block;
};

/**
 * @brief CRTP helper class that implements asset ref
 * implicit/explicit conversion boilerplate
 *
 * Usage:
 *
 * class RandomRef : public<RandomAssetRef, RandomAsset> {
 * public:
 *     using AssetRefMixin::AssetRefMixin;
 *
 *     // then insert your custom methods here
 *     int rng() const { ... }
 * }
 *
 * // now you are able to do this:
 * AssetRef<RandomAsset> asset;
 * RandomRef r = std::move(asset);
 * r.rng()
 */
template <typename Derived, typename TAsset>
class AssetRefMixin : public AssetRef<TAsset> {
public:
    using AssetRef<TAsset>::AssetRef;
    AssetRefMixin() = default;

    /* move constructor/assignment */
    AssetRefMixin(AssetRef<TAsset> &&other)
        : AssetRef<TAsset>(std::move(other)) {}
    AssetRefMixin(const AssetRef<TAsset> &other)
        : AssetRef<TAsset>(other) {}

    /* copy constructor/assignment */
    AssetRefMixin &operator=(AssetRef<TAsset> &&rhs) {
        AssetRef<TAsset>::operator=(std::move(rhs));
        return *this;
    }
    AssetRefMixin &operator=(const AssetRef<TAsset> &rhs) {
        AssetRef<TAsset>::operator=(rhs);
        return *this;
    }
};

#include "asset_ref.inl.h" // IWYU pragma: keep
