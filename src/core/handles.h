#pragma once

#include <cstdint>
#include <optional>
#include <queue>
#include <vector>

#include "assertion.h"

#define HANDLE_UPPER (0xFFFF0000)
#define HANDLE_LOWER (0x0000FFFF)
#define HANDLE_BITSHIFT (16)

using Handle = uint32_t;

template <typename T>
class HandlePool;

template <typename T>
class HandleDestructor {
public:
    operator const T *() const {
        return m_object;
    }

    operator T *() {
        return m_object;
    }

    T *operator->() {
        return m_object;
    }

    const T *operator->() const {
        return m_object;
    }

    ~HandleDestructor() {
        ASSERT(m_pool, "Pool does not exist!");
        m_pool->remove(m_handle);
    }

    HandleDestructor(HandleDestructor &other) = delete;
    HandleDestructor(HandleDestructor &&other) = delete;
    HandleDestructor &operator=(HandleDestructor &rhs) = delete;
    HandleDestructor &operator=(HandleDestructor &&rhs) = delete;

private:
    HandleDestructor(HandlePool<T> *pool, T *object, Handle handle)
        : m_pool(pool), m_object(object), m_handle(handle) {}

    HandlePool<T> *m_pool;
    T *m_object;
    Handle m_handle;
    friend HandlePool<T>;
};

template <typename T>
class HandlePool {
public:
    template <typename... Args>
    Handle emplace(Args &&...args) {
        Slot *slot = reserve_slot();
        slot->data.emplace(std::forward<Args>(args)...);
        return slot->handle;
    }

    const T *get(Handle handle) const {
        const Slot *slot = get_slot(handle);
        if (!slot) {
            return nullptr;
        }

        ASSERT(slot->data.has_value(), "Attempted to get a dead slot!");
        return &slot->data.value();
    }

    T *get(Handle handle) {
        const auto *const_this = static_cast<const HandlePool<T> *>(this);
        return const_cast<T *>(
            const_this->get(handle));
    }

    /** @brief Gets handle using RAII principles.
     *
     * @param handle Handle to obtain handle from. Must be an object that exists!
     *
     * @return RAII-styled object that automatically calls this->remove() on destructor. */
    HandleDestructor<T> get_and_destroy(Handle handle) {
        T *obj = this->get(handle);
        ASSERT(obj);
        return {this, obj, handle};
    }

    [[nodiscard]]
    bool has(Handle handle) const {
        return get_slot(handle) != nullptr;
    }

    void remove(Handle handle) {
        Slot *slot = const_cast<Slot *>(this->get_slot(handle));
        if (!slot) {
            ASSERT(slot, "Attempted to remove a non-existing handle!");
            return;
        }

        slot->handle = handle_add_generation(slot->handle, 1);
        slot->data.reset();

        auto index = static_cast<size_t>(slot - m_pool.data());
        m_available_slots.emplace(index);
    }

    void clear() {
        m_pool.clear();
        while (!m_available_slots.empty()) {
            m_available_slots.pop();
        }
    }

private:
    static size_t handle_get_index(Handle handle) {
        return (handle & HANDLE_UPPER) >> HANDLE_BITSHIFT;
    }

    static Handle handle_from_index(size_t index) {
        return static_cast<Handle>(index << HANDLE_BITSHIFT);
    }

    static size_t handle_get_generation(Handle handle) {
        return static_cast<size_t>(handle & HANDLE_LOWER);
    }

    static Handle handle_add_generation(Handle handle, int amount) {
        return static_cast<Handle>(
            (handle & HANDLE_UPPER) |
            ((handle + amount) & HANDLE_LOWER));
    }

private:
    struct Slot {
        std::optional<T> data;
        Handle handle;
    };
    std::vector<Slot> m_pool;
    std::queue<size_t> m_available_slots;

private:
    [[nodiscard]]
    Slot *reserve_slot() {
        Slot *slot = nullptr;
        if (!m_available_slots.empty()) {
            /* recycle available slot */
            size_t index = m_available_slots.front();
            m_available_slots.pop();
            slot = &m_pool.at(index);
        } else {
            /* allocate new slot */
            size_t index = m_pool.size();
            m_pool.emplace_back();
            slot = &m_pool.at(index);
            slot->handle = handle_from_index(index);
        }
        return slot;
    }

    [[nodiscard]]
    const Slot *get_slot(Handle handle) const {
        size_t index = handle_get_index(handle);

        if (index >= m_pool.size()) {
            return nullptr;
        }

        const Slot &slot = m_pool.at(index);
        if (!slot.data.has_value()) {
            return nullptr;
        }

        if (slot.handle != handle) {
            return nullptr;
        }

        return &slot;
    }
};
