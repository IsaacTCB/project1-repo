#pragma once

#include <array>
#include <atomic>
#include <cstddef>

template <typename T, size_t Capacity>
class SpscQueue {
public:
    template <typename... Args>
    bool try_emplace(Args &&...args) {
        const size_t tail = m_tail.load(std::memory_order_relaxed);
        const size_t next = (tail + 1) % Capacity;

        if (next == m_head.load(std::memory_order_acquire)) {
            return false;
        }

        m_data[tail] = T(std::forward<Args>(args)...);

        m_tail.store(next, std::memory_order_release);
        return true;
    }

    bool try_push(const T &value) {
        return this->try_emplace(value);
    }

    bool try_push(T &&value) {
        return this->try_emplace(std::move(value));
    }

    bool try_pop(T &value) {
        const size_t head = m_head.load(std::memory_order_relaxed);
        const size_t next = (head + 1) % Capacity;

        if (head == m_tail.load(std::memory_order_acquire)) {
            return false;
        }

        value = std::move(m_data[head]);

        m_head.store(next, std::memory_order_release);
        return true;
    }

    [[nodiscard]]
    bool empty() const noexcept {
        size_t head = m_head.load(std::memory_order_relaxed);
        return head == m_tail.load(std::memory_order_acquire);
    }

    [[nodiscard]]
    size_t size() const noexcept {
        const std::size_t head = m_head.load(std::memory_order_acquire);
        const std::size_t tail = m_tail.load(std::memory_order_acquire);

        if (tail >= head) {
            return tail - head;
        }

        return Capacity - head + tail;
    }

private:
    std::array<T, Capacity> m_data{};
    std::atomic<size_t> m_tail{0};
    std::atomic<size_t> m_head{0};
};
