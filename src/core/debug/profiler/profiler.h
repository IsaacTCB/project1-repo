#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <source_location>
#include <vector>

/* ==================== *
 *        Scope         *
 * ==================== */

class Profiler;

struct ProfilerEvent {
    std::source_location source;
    const char *tag;
    size_t thread_id;
    uint64_t start_ns;
    uint64_t duration_ns;
};

struct ProfilerFrame {
    std::vector<ProfilerEvent> events;
    size_t main_thread_id; /* used by the profiler window */
    size_t dropped;        /* number of scoped events whose push was rejected */
};

class ProfilerScope {
protected:
    friend Profiler;
    explicit ProfilerScope(Profiler *profiler, const char *tag, const std::source_location &source)
        : m_profiler(profiler), m_tag(tag), m_source(source) {
        begin_profiler();
    }

public:
    ProfilerScope() = default;
    ~ProfilerScope() {
        if (m_profiler) {
            end_profiler();
        }
    }
    ProfilerScope(const ProfilerScope &rhs) = delete;
    ProfilerScope(ProfilerScope &&rhs) = delete;
    ProfilerScope &operator=(const ProfilerScope &rhs) = delete;
    ProfilerScope &operator=(ProfilerScope &&rhs) = delete;

private:
    void begin_profiler();
    void end_profiler();

private:
    Profiler *m_profiler{};
    const char *m_tag;
    std::source_location m_source;

    /* counter */
    std::chrono::steady_clock::time_point m_begin_timestamp;
};

/* ==================== *
 *       Profiler       *
 * ==================== */

class Profiler {
public:
    static Profiler &get();

public:
    /**
     * @brief Starts a new profiler frame.
     *
     * @param frame The frame that will be recorded into.
     */
    void begin_frame(ProfilerFrame &&frame);

    [[nodiscard]]
    ProfilerFrame end_frame();

    [[nodiscard]]
    ProfilerScope scope(const char *tag, const std::source_location &source = std::source_location::current());

protected:
    void record(const ProfilerEvent &event);
    friend ProfilerScope;

private:
    ProfilerFrame m_current_frame;
    std::mutex m_mutex;
};

/* ==================== *
 *       Macros         *
 * ==================== */

#if ENABLE_PROFILER

inline constexpr bool PROFILER_ENABLED = true;
#define PROFILE_CONCAT_HIDDEN(a, b) a##b
#define PROFILE_CONCAT(a, b) PROFILE_CONCAT_HIDDEN(a, b)
#define PROFILE_STRINGIFY(x) #x

#define PROFILE_FUNCTION_SCOPE() \
    const auto PROFILE_CONCAT(profiler__func_, __LINE__) = Profiler::get().scope(nullptr)
#define PROFILE_TAGGED_SCOPE(tag) \
    const auto PROFILE_CONCAT(profiler__tagged_, tag) = Profiler::get().scope(PROFILE_STRINGIFY(tag))

#else

inline constexpr bool PROFILER_ENABLED = false;
#define PROFILE_FUNCTION_SCOPE()
#define PROFILE_TAGGED_SCOPE(tag)

#endif /* ENABLE_PROFILER */
