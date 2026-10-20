#include "profiler.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <thread>

#include "profiler_limits.h"

/* ==================== *
 *        Scope         *
 * ==================== */

void ProfilerScope::begin_profiler() {
    m_begin_timestamp = std::chrono::steady_clock::now();
};

void ProfilerScope::end_profiler() {
    const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now() - m_begin_timestamp);
    const auto start = std::chrono::time_point_cast<std::chrono::nanoseconds>(m_begin_timestamp);

    const auto d = static_cast<uint64_t>(duration.count());
    const auto s = static_cast<uint64_t>(start.time_since_epoch().count());
    const auto thid = std::this_thread::get_id();

    m_profiler->record({
        .source = m_source,
        .thread_id = std::hash<std::thread::id>{}(thid),
        .start_ns = s,
        .duration_ns = d,
    });
};

/* ==================== *
 *       Profiler       *
 * ==================== */

Profiler &Profiler::get() {
    static Profiler s_profiler;
    return s_profiler;
}

void Profiler::begin_frame(ProfilerFrame &&frame) {
    const auto lock = std::scoped_lock{m_mutex};
    m_current_frame = std::move(frame);
    m_current_frame.events.resize(0);
    m_current_frame.dropped = 0;
}

ProfilerFrame Profiler::end_frame() {
    const auto lock = std::scoped_lock{m_mutex};
    return std::move(m_current_frame);
}

ProfilerScope Profiler::scope(const std::source_location &source) {
    return ProfilerScope{this, source};
}

void Profiler::record(const ProfilerEvent &event) {
    const auto lock = std::scoped_lock{m_mutex};
    if (m_current_frame.events.size() >= ProfilerLimits::MAX_EVENTS_PER_FRAME) {
        ++m_current_frame.dropped;
        return;
    }
    m_current_frame.events.push_back(event);
}
