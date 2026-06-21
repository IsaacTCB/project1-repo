#pragma once

#include <cstdint>
#include <format>
#include <limits>
#include <optional>
#include <queue>
#include <type_traits>
#include <vector>

#include "assertion.h"

struct Handle {
public:
    using StorageType = uint32_t;
    static_assert(std::is_unsigned_v<StorageType> &&
                  std::is_integral_v<StorageType> &&
                  std::numeric_limits<StorageType>::is_integer);

public:
    constexpr Handle() = default;

    [[nodiscard]] static constexpr Handle from_index_gen(size_t index, size_t gen) noexcept {
        if (index > INDEX_MAX || index == INDEX_INVALID) {
            return Handle{};
        }

        auto converted_idx = static_cast<StorageType>(index) + 1;
        auto value = static_cast<StorageType>(converted_idx << GENERATION_BITS);
        value |= gen & GENERATION_MASK;
        return Handle{value};
    }

    [[nodiscard]] static constexpr Handle from_index(size_t index) noexcept {
        return from_index_gen(index, 0);
    }

public:
    [[nodiscard]] constexpr size_t index() const noexcept {
        if (!valid()) {
            return INDEX_INVALID;
        }
        return ((m_value & INDEX_MASK) >> GENERATION_BITS) - 1;
    }

    [[nodiscard]] constexpr size_t gen() const noexcept {
        return (m_value & GENERATION_MASK);
    }

    [[nodiscard]] constexpr StorageType value() const noexcept {
        return m_value;
    }

    [[nodiscard]] constexpr bool valid() const noexcept {
        return (m_value & INDEX_MASK) != 0;
    }

public:
    [[nodiscard]] constexpr Handle inc_gen() const {
        StorageType new_value = (m_value + 1) & GENERATION_MASK;
        new_value |= m_value & INDEX_MASK;
        return Handle{new_value};
    }

    [[nodiscard]] constexpr Handle dec_gen() const {
        StorageType new_value = (m_value - 1) & GENERATION_MASK;
        new_value |= m_value & INDEX_MASK;
        return Handle{new_value};
    }

public:
    /* Conversion operators */

    [[nodiscard]] constexpr explicit operator StorageType() const noexcept {
        return value();
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return valid();
    }

public:
    /* Comparison operators */

    constexpr bool operator==(const Handle &other) const noexcept {
        return this->m_value == other.m_value;
    }

    constexpr bool operator!=(const Handle &other) const noexcept {
        return !(*this == other);
    }

private:
    constexpr explicit Handle(StorageType value)
        : m_value(value) {}

public:
    /* Split point between upper and lower bits */
    static constexpr StorageType GENERATION_BITS =
        std::numeric_limits<StorageType>::digits / 2;

    /* Upper bits stores index */
    static constexpr auto INDEX_MASK = static_cast<StorageType>(
        ~StorageType{0} << GENERATION_BITS);

    /* Lower bits stores generation */
    static constexpr auto GENERATION_MASK = static_cast<StorageType>(
        ~INDEX_MASK << 0);

    static constexpr auto INDEX_INVALID = std::numeric_limits<size_t>::max();
    static constexpr auto INDEX_MAX = static_cast<size_t>(INDEX_MASK >> GENERATION_BITS) - 1;
    static constexpr auto GENERATION_MAX = static_cast<size_t>(GENERATION_MASK);

private:
    StorageType m_value;
};

template <>
struct std::formatter<Handle> : std::formatter<std::string_view> {
    auto format(const Handle &vec, format_context &ctx) const {
        std::string s;
        if (vec.valid()) {
            s = std::format("({}:{})", vec.index(), vec.gen());
        } else {
            s = "(null)";
        }
        return formatter<std::string_view>::format(s, ctx);
    }
};

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
        return slot ? &slot->data.value()
                    : nullptr;
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

        slot->handle = slot->handle.inc_gen();
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
            auto handle = Handle::from_index(index);
            if (handle) {
                m_pool.emplace_back();
                slot = &m_pool.at(index);
                slot->handle = handle;
            } else {
                ASSERT(false, "Handle::from_index() failed! Attempted to allocate invalid handle!")
            }
        }
        return slot;
    }

    [[nodiscard]]
    const Slot *get_slot(Handle handle) const {
        if (!handle) {
            return nullptr;
        }

        size_t index = handle.index();
        ASSERT(index != Handle::INDEX_INVALID);

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
