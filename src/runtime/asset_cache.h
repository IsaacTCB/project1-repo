#pragma once

#include <atomic>
#include <functional>
#include <string_view>
#include <unordered_map>

#include "core/assertion.h"
#include "core/handles.h"

// ===========

class AssetSlotBase {
public:
    explicit AssetSlotBase()
        : ref_count{0} {}

    void inc_ref() {
        ref_count++;
    }

    void dec_ref() {
        if (ref_count == 0 || --ref_count == 0) {
            if (destroy_callback) {
                destroy_callback(this);
            }
        }
    }

public:
    std::atomic_size_t ref_count;
    void (*destroy_callback)(AssetSlotBase *slot){};
};

// ===========

/* forward declaration */
template <typename T>
class AssetCache;

template <typename Handle>
class AssetSlot : public AssetSlotBase {
public:
    explicit AssetSlot()
        : AssetSlotBase() {}

    explicit AssetSlot(AssetCache<Handle> *cache, size_t hash, Handle handle)
        : AssetSlotBase(), cache(cache), hash(hash), handle(handle) {}

public:
    AssetCache<Handle> *cache;
    size_t hash;
    Handle handle;
};

// ===========

template <typename T>
class AssetRef {
public:
    explicit AssetRef() : m_slot(nullptr), m_handle{} {}

    ~AssetRef() {
        release();
    }

    void reset() {
        *this = AssetRef<T>{};
    }

    void acquire() {
        if (m_slot) {
            m_slot->inc_ref();
        }
    }

    void release() {
        if (m_slot) {
            auto *slot = m_slot;
            m_slot = nullptr;
            slot->dec_ref();
        }
    }

    T get() const {
        ASSERT(m_slot, "Attempted to use get() on an invalid asset ref.");
        return m_handle;
    }

    AssetRef(const AssetRef &other)
        : m_slot(other.m_slot), m_handle(other.m_handle) {
        acquire();
    }

    AssetRef(AssetRef &&other) noexcept
        : m_slot(other.m_slot), m_handle(other.m_handle) {
        other.m_slot = nullptr;
        other.m_handle = {};
    }

    AssetRef &operator=(const AssetRef<T> &rhs) {
        if (this == &rhs) {
            return *this;
        }

        release();
        m_slot = rhs.m_slot;
        m_handle = rhs.m_handle;
        acquire();
        return *this;
    }

    AssetRef &operator=(AssetRef<T> &&rhs) noexcept {
        if (this == &rhs) {
            return *this;
        }

        release();
        m_slot = rhs.m_slot;
        m_handle = rhs.m_handle;
        rhs.m_slot = nullptr;
        rhs.m_handle = T{};
        return *this;
    }

private:
    AssetSlotBase *m_slot;
    T m_handle;

private:
    explicit AssetRef(AssetSlotBase *slot, Handle handle)
        : m_slot(slot), m_handle(handle) {}

    friend class AssetCache<T>;
};

// ===========

template <typename T>
class AssetCache {
private:
    static void destroy(AssetSlotBase *slot) {
        auto *asset_slot = static_cast<AssetSlot<T> *>(slot);
        auto *asset_cache = asset_slot->cache;
        if (asset_cache->unload) {
            asset_cache->unload(asset_slot->handle);
        }
        asset_cache->m_cache.erase(asset_slot->hash);
    }

public:
    AssetRef<T> get(std::string_view path) {
        size_t hash = get_hash(path);
        AssetSlot<T> *slot;
        if (auto it = m_cache.find(hash); it != m_cache.end()) {
            slot = &it->second;
        } else {
            T handle = load(path);
            auto [emplaced, success] = m_cache.try_emplace(hash, this, hash, handle);

            ASSERT(success, "Failed to emplace asset slot");
            slot = &emplaced->second;
        }

        slot->ref_count++;
        slot->destroy_callback = AssetCache<T>::destroy;
        return AssetRef<T>{slot, slot->handle};
    }

    static size_t get_hash(std::string_view path) {
        static std::hash<std::string_view> hash_generator{};
        return hash_generator(path);
    }

    using LoadFn = std::function<Handle(std::string_view)>;
    LoadFn load;

    using UnloadFn = std::function<void(Handle)>;
    UnloadFn unload;

private:
    std::unordered_map<size_t, AssetSlot<T>> m_cache;
};
