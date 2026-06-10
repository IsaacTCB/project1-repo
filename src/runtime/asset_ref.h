#pragma once

#include <atomic>
#include <cstddef>

/**
 * @brief An shared interface used by AssetRef to
 * reference an asset. An implementation class is
 * used inside AssetCache.
 *
 * @tparam Handle The underlying asset/handle.
 */
template <typename Handle>
class IAssetControlBlock {
public:
    using DestroyFn = void (*)(IAssetControlBlock *caller);
    DestroyFn destroy_fn;

protected:
    explicit IAssetControlBlock();
    explicit IAssetControlBlock(DestroyFn destroy_fn, Handle handle);

public:
    void inc_ref();
    void dec_ref();
    Handle get_handle();

public:
    std::atomic<size_t> ref_count;
    Handle handle;
};

/* forward declaration */
template <typename Handle>
class AssetCache;

/**
 * @brief A shared resource reference that holds an
 * asset Handle.
 *
 * @note Users are not meant to construct this object
 * directly, only rather via a wrapper service like
 * AssetCache.
 *
 * On the surface, it behaves similarly to std::shared_ptr,
 * but internally it's a little bit more specialized and
 * thus **should** result in better performance.
 *
 * @tparam T Handle type
 */
template <typename Handle>
class AssetRef {
private:
    friend class AssetCache<Handle>;
    explicit AssetRef(IAssetControlBlock<Handle> &slot);

public:
    explicit AssetRef();
    ~AssetRef();

    AssetRef(const AssetRef &other);
    AssetRef(AssetRef &&other) noexcept;
    AssetRef &operator=(const AssetRef<Handle> &rhs);
    AssetRef &operator=(AssetRef<Handle> &&rhs) noexcept;

public:
    void reset();
    void acquire();
    void release();
    Handle get() const;

private:
    IAssetControlBlock<Handle> *m_ctrl_block;
    Handle m_handle;
};

#include "asset_ref.inl.h" // IWYU pragma: keep
