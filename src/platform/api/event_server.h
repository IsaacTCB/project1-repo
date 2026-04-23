#include "events.h"

#include <span>

class EventServer {
public:
    template <typename T, typename... Args>
    void submit(Args &&...args) {
        m_events.emplace<T>(std::forward<Args>(args)...);
    }

    template <typename T>
    std::span<T> span() {
        return m_events.span<T>();
    }

private:
    friend class Platform;
    EventQueue m_events;
};
