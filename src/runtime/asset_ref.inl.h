#pragma once

#include "asset_ref.h"

/* == Asset Control Block == */

template <typename Handle>
IAssetControlBlock<Handle>::IAssetControlBlock() = default;

template <typename Handle>
IAssetControlBlock<Handle>::IAssetControlBlock(DestroyFn destroy_fn, Handle handle)
    : destroy_fn(destroy_fn), ref_count(0), handle(handle) {}

template <typename Handle>
void IAssetControlBlock<Handle>::inc_ref() {
    ref_count++;
}

template <typename Handle>
void IAssetControlBlock<Handle>::dec_ref() {
    if (ref_count == 0 || --ref_count == 0) {
        destroy_fn(this);
    }
}

template <typename Handle>
Handle IAssetControlBlock<Handle>::get_handle() {
    return handle;
}

/* == Asset Ref == */

template <typename Handle>
AssetRef<Handle>::AssetRef(IAssetControlBlock<Handle> &slot)
    : m_ctrl_block(&slot), m_handle(slot.get_handle()) {}

template <typename Handle>
AssetRef<Handle>::AssetRef() : m_ctrl_block(nullptr), m_handle{} {}

template <typename Handle>
AssetRef<Handle>::~AssetRef() {
    release();
}

template <typename Handle>
AssetRef<Handle>::AssetRef(const AssetRef &other)
    : m_ctrl_block(other.m_ctrl_block), m_handle(other.m_handle) {
    acquire();
}

template <typename Handle>
AssetRef<Handle>::AssetRef(AssetRef &&other) noexcept
    : m_ctrl_block(other.m_ctrl_block), m_handle(other.m_handle) {
    other.m_ctrl_block = nullptr;
    other.m_handle = Handle{};
}

template <typename Handle>
AssetRef<Handle> &AssetRef<Handle>::operator=(const AssetRef<Handle> &rhs) {
    if (this == &rhs) {
        return *this;
    }

    release();
    m_ctrl_block = rhs.m_ctrl_block;
    m_handle = rhs.m_handle;
    acquire();
    return *this;
}

template <typename Handle>
AssetRef<Handle> &AssetRef<Handle>::operator=(AssetRef<Handle> &&rhs) noexcept {
    if (this == &rhs) {
        return *this;
    }

    release();
    m_ctrl_block = rhs.m_ctrl_block;
    m_handle = rhs.m_handle;
    rhs.m_ctrl_block = nullptr;
    rhs.m_handle = Handle{};
    return *this;
}

template <typename Handle>
void AssetRef<Handle>::reset() {
    *this = AssetRef<Handle>{};
}

template <typename Handle>
void AssetRef<Handle>::acquire() {
    if (m_ctrl_block) {
        m_ctrl_block->inc_ref();
    }
}

template <typename Handle>
void AssetRef<Handle>::release() {
    if (m_ctrl_block) {
        auto *slot = m_ctrl_block;
        m_ctrl_block = nullptr;
        slot->dec_ref();
    }
}

template <typename Handle>
Handle AssetRef<Handle>::get() const {
    return m_handle;
}
