#pragma once

#include <functional>
#include <optional>
#include <queue>
#include <vector>

#include "core/assertion.h"

#include "handles.h"

template <typename T, typename HandleType>
    requires is_handle_v<HandleType>
class HandlePool;

template <typename T, typename HandleType>
    requires is_handle_v<HandleType>
class HandleDestructor {
private:
    static_assert(is_handle_v<HandleType>);
    using PoolType = HandlePool<T, HandleType>;

public:
    ~HandleDestructor() {
        if (this->valid()) {
            m_pool->remove(m_handle);
        }
    }

    HandleDestructor(HandleDestructor &&other) noexcept
        : m_pool(other.m_pool),
          m_object(other.m_object),
          m_handle(other.m_handle) {
        other.m_pool = nullptr;
        other.m_object = nullptr;
        other.m_handle = HandleType{};
    }

    HandleDestructor &operator=(HandleDestructor &&rhs) noexcept {
        if (this == &rhs) {
            return *this;
        }

        reset();

        m_pool = rhs.m_pool;
        m_object = rhs.m_object;
        m_handle = rhs.m_handle;

        rhs.m_pool = nullptr;
        rhs.m_object = nullptr;
        rhs.m_handle = HandleType{};

        return *this;
    }

    HandleDestructor(HandleDestructor &other) = delete;
    HandleDestructor &operator=(HandleDestructor &rhs) = delete;

    [[nodiscard]] bool valid() const noexcept {
        return (m_pool && m_object && m_handle);
    }

public:
    [[nodiscard]] explicit operator bool() const noexcept {
        return valid();
    }

    [[nodiscard]] operator T *() const {
        return m_object;
    }

    [[nodiscard]] T *operator->() const {
        ASSERT(m_object, "Attempted to access object from an invalid HandleDestructor<T>!");
        return m_object;
    }

private:
    void reset() noexcept {
        if (valid()) {
            m_pool->remove(m_handle);
        }

        m_pool = nullptr;
        m_object = nullptr;
        m_handle = HandleType{};
    }

private:
    explicit HandleDestructor() = default;
    explicit HandleDestructor(PoolType *pool, T *object, HandleType handle)
        : m_pool(pool), m_object(object), m_handle(handle) {}

    PoolType *m_pool{};
    T *m_object{};
    HandleType m_handle;
    friend PoolType;
};

template <typename T, typename HandleType>
    requires is_handle_v<HandleType>
class HandlePool {
private:
    using DestructorType = HandleDestructor<T, HandleType>;

public:
    template <typename... Args>
    HandleType emplace(Args &&...args) {
        Slot *slot = reserve_slot();
        slot->data.emplace(std::forward<Args>(args)...);
        return slot->next_handle;
    }

    HandleType push(const T &value) {
        return this->emplace(value);
    }

    HandleType push(T &&value) {
        return this->emplace(value);
    }

    const T *get(HandleType handle) const {
        const Slot *slot = get_slot(handle);
        return slot ? &slot->data.value()
                    : nullptr;
    }

    T *get(HandleType handle) {
        const auto *const_this = static_cast<const HandlePool<T, HandleType> *>(this);
        return const_cast<T *>(
            const_this->get(handle));
    }

    /** @brief Gets handle using RAII principles.
     *
     * This is typically used by unload functions to
     * manage object destruction safely.
     *
     * It's imperative that after obtaining a destructor,
     * you DON'T call remove().
     *
     * @param handle HandleType to obtain handle from. Must be an object that exists!
     *
     * @return RAII-styled object that automatically calls this->remove() on destructor. */
    DestructorType get_and_destroy(HandleType handle) {
        T *obj = this->get(handle);
        return obj ? DestructorType{this, obj, handle}
                   : DestructorType{};
    }

    [[nodiscard]]
    bool has(HandleType handle) const {
        return get_slot(handle) != nullptr;
    }

    void remove(HandleType handle) {
        Slot *slot = const_cast<Slot *>(this->get_slot(handle));
        if (!slot) {
            ASSERT(slot, "Attempted to remove a non-existing handle!");
            return;
        }

        slot->next_handle = slot->next_handle.inc_gen();
        slot->data.reset();

        auto index = static_cast<size_t>(slot - m_pool.data());
        m_available_slots.emplace(index);
    }

    template <typename Func>
    void foreach (Func &&func) {
        for (Slot &slot : m_pool) {
            if (!slot.data.has_value()) {
                continue;
            }

            std::invoke(func, *slot.data);
        }
    }

    void clear() {
        m_pool.clear();
        while (!m_available_slots.empty()) {
            m_available_slots.pop();
        }
    }

private:
    struct Slot {
        std::optional<T> data;
        HandleType next_handle;
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
            if (auto handle = HandleType::from_index(index); handle) {
                m_pool.emplace_back();
                slot = &m_pool.at(index);
                slot->next_handle = handle;
            } else {
                ASSERT(false, "Handle::from_index() failed! Attempted to allocate invalid handle!")
            }
        }
        return slot;
    }

    [[nodiscard]]
    const Slot *get_slot(HandleType handle) const {
        if (!handle) {
            return nullptr;
        }

        size_t index = handle.index();
        ASSERT(index != HandleType::INDEX_INVALID);

        if (index >= m_pool.size()) {
            return nullptr;
        }

        const Slot &slot = m_pool.at(index);
        if (!slot.data.has_value()) {
            return nullptr;
        }

        if (slot.next_handle != handle) {
            return nullptr;
        }

        return &slot;
    }
};
