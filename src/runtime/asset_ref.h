#pragma once

#include <atomic>
#include <cstddef>

template <typename Handle>
class IAssetControlBlock {
public:
    explicit IAssetControlBlock() = default;
    explicit IAssetControlBlock(Handle handle)
        : ref_count(0), handle(handle) {}

    virtual void destroy() = 0;

    void inc_ref() {
        ref_count++;
    }

    void dec_ref() {
        if (ref_count == 0 || --ref_count == 0) {
            destroy();
        }
    }

    Handle get_handle() {
        return handle;
    }

public:
    std::atomic<size_t> ref_count;
    Handle handle;
};

// ===========

/* forward declaration */
template <typename T>
class AssetCache;

template <typename T>
class AssetRef {
public:
    explicit AssetRef() : m_ctrl_block(nullptr), m_handle{} {}
    ~AssetRef() {
        release();
    }

    AssetRef(const AssetRef &other)
        : m_ctrl_block(other.m_ctrl_block), m_handle(other.m_handle) {
        acquire();
    }

    AssetRef(AssetRef &&other) noexcept
        : m_ctrl_block(other.m_ctrl_block), m_handle(other.m_handle) {
        other.m_ctrl_block = nullptr;
        other.m_handle = T{};
    }

    AssetRef &operator=(const AssetRef<T> &rhs) {
        if (this == &rhs) {
            return *this;
        }

        release();
        m_ctrl_block = rhs.m_ctrl_block;
        m_handle = rhs.m_handle;
        acquire();
        return *this;
    }

    AssetRef &operator=(AssetRef<T> &&rhs) noexcept {
        if (this == &rhs) {
            return *this;
        }

        release();
        m_ctrl_block = rhs.m_ctrl_block;
        m_handle = rhs.m_handle;
        rhs.m_ctrl_block = nullptr;
        rhs.m_handle = T{};
        return *this;
    }

public:
    void reset() {
        *this = AssetRef<T>{};
    }

    void acquire() {
        if (m_ctrl_block) {
            m_ctrl_block->inc_ref();
        }
    }

    void release() {
        if (m_ctrl_block) {
            auto *slot = m_ctrl_block;
            m_ctrl_block = nullptr;
            slot->dec_ref();
        }
    }

    T get() const {
        return m_handle;
    }

private:
    friend class AssetCache<T>;
    explicit AssetRef(IAssetControlBlock<T> &slot)
        : m_ctrl_block(&slot), m_handle(slot.get_handle()) {}

private:
    IAssetControlBlock<T> *m_ctrl_block;
    T m_handle;
};
