#pragma once

#include "asset_ref.h"

/* == Asset Control Block == */

template <typename TAsset>
IAssetControlBlock<TAsset>::IAssetControlBlock() = default;

template <typename TAsset>
template <typename MakeAsset>
IAssetControlBlock<TAsset>::IAssetControlBlock(DestroyFn destroy_fn, MakeAsset &&make_asset)
    : destroy_fn(destroy_fn), ref_count(0), handle(std::forward<MakeAsset>(make_asset)()) {}

template <typename TAsset>
void IAssetControlBlock<TAsset>::inc_ref() {
    ref_count++;
}

template <typename TAsset>
void IAssetControlBlock<TAsset>::dec_ref() {
    if (ref_count == 0 || --ref_count == 0) {
        destroy_fn(this);
    }
}

template <typename TAsset>
TAsset IAssetControlBlock<TAsset>::get_handle() {
    return handle;
}

/* == Asset Ref == */

template <typename TAsset>
AssetRef<TAsset>::AssetRef(IAssetControlBlock<TAsset> &slot)
    : m_ctrl_block(&slot) {}

template <typename TAsset>
AssetRef<TAsset>::AssetRef() : m_ctrl_block(nullptr) {}

template <typename TAsset>
AssetRef<TAsset>::~AssetRef() {
    release();
}

template <typename TAsset>
AssetRef<TAsset>::AssetRef(const AssetRef &other)
    : m_ctrl_block(other.m_ctrl_block) {
    acquire();
}

template <typename TAsset>
AssetRef<TAsset>::AssetRef(AssetRef &&other) noexcept
    : m_ctrl_block(other.m_ctrl_block) {
    other.m_ctrl_block = nullptr;
}

template <typename TAsset>
AssetRef<TAsset> &AssetRef<TAsset>::operator=(const AssetRef<TAsset> &rhs) {
    if (this == &rhs) {
        return *this;
    }

    release();
    m_ctrl_block = rhs.m_ctrl_block;
    acquire();
    return *this;
}

template <typename TAsset>
AssetRef<TAsset> &AssetRef<TAsset>::operator=(AssetRef<TAsset> &&rhs) noexcept {
    if (this == &rhs) {
        return *this;
    }

    release();
    m_ctrl_block = rhs.m_ctrl_block;
    rhs.m_ctrl_block = nullptr;
    return *this;
}

template <typename TAsset>
void AssetRef<TAsset>::reset() {
    *this = AssetRef<TAsset>{};
}

template <typename TAsset>
void AssetRef<TAsset>::acquire() {
    if (m_ctrl_block) {
        m_ctrl_block->inc_ref();
    }
}

template <typename TAsset>
void AssetRef<TAsset>::release() {
    if (m_ctrl_block) {
        auto *slot = m_ctrl_block;
        m_ctrl_block = nullptr;
        slot->dec_ref();
    }
}

template <typename TAsset>
const TAsset &AssetRef<TAsset>::get() const {
    static TAsset s_null{};
    if (!m_ctrl_block) {
        return s_null;
    }
    return m_ctrl_block->handle;
}
