#pragma once

#include <atomic>
#include <functional>
#include <string_view>
#include <unordered_map>

#include "core/assertion.h"
#include "core/handles.h"

/* forward declaration */
template <typename T>
class AssetCache;

template <typename Handle>
class AssetSlot {
public:
    explicit AssetSlot() = default;

    explicit AssetSlot(AssetCache<Handle> *cache, size_t hash, Handle handle)
        : ref_count(0), cache(cache), hash(hash), handle(handle) {}

    void inc_ref() {
        ref_count++;
    }

    void dec_ref() {
        if (ref_count == 0 || --ref_count == 0) {
            cache->destroy(this);
        }
    }

    Handle get_handle() {
        return handle;
    }

public:
    std::atomic<size_t> ref_count;
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
        return m_handle;
    }

    AssetRef(const AssetRef &other)
        : m_slot(other.m_slot), m_handle(other.m_handle) {
        acquire();
    }

    AssetRef(AssetRef &&other) noexcept
        : m_slot(other.m_slot), m_handle(other.m_handle) {
        other.m_slot = nullptr;
        other.m_handle = T{};
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
    explicit AssetRef<T>(AssetSlot<T> &slot)
        : m_slot(&slot), m_handle(slot.get_handle()) {}

private:
    AssetSlot<T> *m_slot;
    T m_handle;

    friend class AssetCache<T>;
};

// ===========

template <typename T>
class AssetCache {
public:
    void destroy(AssetSlot<T> *slot) {
        if (this->unload_fn) {
            this->unload_fn(slot->handle);
        }
        this->m_cache.erase(slot->hash);
    }

public:
    AssetRef<T> get(std::string_view path) {
        size_t hash = get_hash(path);
        AssetSlot<T> *slot;
        if (auto it = m_cache.find(hash); it != m_cache.end()) {
            slot = &it->second;
        } else {
            T handle = load_fn(path);
            auto [emplaced, success] = m_cache.try_emplace(hash, this, hash, handle);

            ASSERT(success, "Failed to emplace asset slot");
            slot = success ? &emplaced->second
                           : nullptr;
        }

        if (!slot) {
            /* in case that we weren't
             * able to fetch it */
            return AssetRef<T>{};
        }

        slot->ref_count++;
        return AssetRef<T>{*slot};
    }

    static size_t get_hash(std::string_view path) {
        static std::hash<std::string_view> hash_generator{};
        return hash_generator(path);
    }

    using LoadFn = std::function<Handle(std::string_view)>;
    LoadFn load_fn;

    using UnloadFn = std::function<void(Handle)>;
    UnloadFn unload_fn;

private:
    std::unordered_map<size_t, AssetSlot<T>> m_cache;
};
