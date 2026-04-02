#pragma once

#include <cstdint>
#include <queue>
#include <vector>

#include "assertion.h"

#define HANDLE_UPPER (0xFFFF0000)
#define HANDLE_LOWER (0x0000FFFF)
#define HANDLE_BITSHIFT (16)

using Handle = uint32_t;

template <typename T>
class HandlePool {
public:
    HandlePool()
    {}

    template<typename... Args>
    Handle emplace(Args&& ...args) {
        Slot* slot = reserve_slot();
        slot->value = T{std::forward<Args>(args)...};
        slot->alive = true;
        return slot->handle;
    }

    template<typename T2>
    Handle emplace(T2&& value) {
        Slot* slot = reserve_slot();
        slot->value = std::move(value);
        slot->alive = true;
        return slot->handle;
    }

    const T* get(Handle handle) const {
        const Slot* slot = get_slot(handle);
        if (!slot) {
            return nullptr;
        }

        ASSERT(slot->alive, "Attempted to get a dead slot!");
        return &slot->value;
    }

    T* get(Handle handle) {
        const auto* const_this = static_cast<const HandlePool<T>*>(this);
        return const_cast<T*>(
            const_this->get(handle)
        );
    }

    bool has(Handle handle) const {
        return get_slot(handle) != nullptr;
    }

    void remove(Handle handle) {
        Slot* slot = const_cast<Slot*>(this->get_slot(handle));
        if (!slot) {
            ASSERT(slot, "Attempted to remove a non-existing handle!");
            return;
        }

        slot->handle = handle_add_generation(slot->handle, 1);
        slot->alive = false;

        size_t index = static_cast<size_t>(slot - m_pool.data());
        m_available_slots.emplace(index);
    }

    void clear() {
        // for (Slot& slot : m_pool) {
        //     if (!slot.alive) { continue; }
        //     /* execute clear operation here */
        // }

        m_pool.clear();
        while (!m_available_slots.empty()) {
            m_available_slots.pop();
        }
    }

private:
    inline static size_t handle_get_index(Handle handle) {
        return (handle & HANDLE_UPPER) >> HANDLE_BITSHIFT;
    }

    inline static Handle handle_from_index(size_t index) {
        return static_cast<Handle>(index << HANDLE_BITSHIFT);
    }

    inline static size_t handle_get_generation(Handle handle) {
        return static_cast<size_t>(handle & HANDLE_LOWER);
    }

    inline static Handle handle_add_generation(Handle handle, int amount) {
        return static_cast<Handle>(
            handle & HANDLE_UPPER |
            ((handle + amount) & HANDLE_LOWER)
        );
    }

private:
    struct Slot {
        T value;
        Handle handle;
        bool alive;
    };
    std::vector<Slot> m_pool;
    std::queue<size_t> m_available_slots;

private:
    [[nodiscard]]
    Slot* reserve_slot() {
        Slot* slot = nullptr;
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
    const Slot* get_slot(Handle handle) const {
        size_t index = handle_get_index(handle);

        if (index >= m_pool.size()) {
            return nullptr;
        }

        const Slot& slot = m_pool.at(index);
        if (!slot.alive) {
            return nullptr;
        }

        if (slot.handle != handle) {
            return nullptr;
        }

        return &slot;
    }
};
