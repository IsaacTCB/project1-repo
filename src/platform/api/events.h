#pragma once

#include "core/assertion.h"

#include <memory>
#include <span>
#include <typeindex>
#include <unordered_map>
#include <vector>

class EventQueue {
public:
    template <typename T, typename... Args>
    void emplace(Args &&...args) {
        auto *pool = get_pool<T>();
        ASSERT(pool != nullptr, "Pool does not exist");
        pool->push(std::forward<Args>(args)...);
    }

    template <typename T>
    std::span<T> span() {
        auto *pool = get_pool<T>();
        ASSERT(pool != nullptr, "Pool does not exist");
        return std::span<T>{pool->data};
    }

    void clear() {
        for (auto &[k, v] : m_pools) {
            v->clear();
        }
    }

private:
    class IEventPool {
    public:
        virtual ~IEventPool() = default;
        virtual void clear() = 0;
    };

    template <typename T>
    class EventPool : public IEventPool {
    public:
        explicit EventPool() = default;

        template <typename... Args>
        void push(Args &&...args) {
            data.emplace_back(std::forward<Args>(args)...);
        }

        void clear() override {
            data.clear();
        }

        std::vector<T> data;
    };

    template <typename T>
    EventPool<T> *get_pool() {
        std::type_index idx{typeid(T)};
        auto it = m_pools.find(idx);

        IEventPool *result = nullptr;
        if (it == m_pools.end()) {
            auto pool = std::unique_ptr<IEventPool>(new EventPool<T>());
            result = pool.get();
            m_pools[idx] = std::move(pool);
        } else {
            result = it->second.get();
        }

        return static_cast<EventPool<T> *>(result);
    }

    std::unordered_map<std::type_index, std::unique_ptr<IEventPool>> m_pools;
};
