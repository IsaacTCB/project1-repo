#include "logger.h"

#include <iostream>
#include <memory>

#include <mutex>
#include <string_view>
static std::mutex s_mutex;

Logger &Logger::get() {
    static Logger s_logger;
    return s_logger;
}

void Logger::add_sink(std::unique_ptr<ILogSink> sink) {
    auto lock = std::scoped_lock(s_mutex);
    m_sinks.push_back(std::move(sink));
}

void Logger::set_sink_override(std::unique_ptr<ILogSink> sink) {
    auto lock = std::scoped_lock(s_mutex);
    m_sink_override = std::move(sink);
}

static inline std::string_view log_prefix(LogSeverity severity) {
    switch (severity) {
        case LOG_SEVERITY_TRACE:
            return "[TRACE] ";
        case LOG_SEVERITY_INFO:
            return "[INFO] ";
        case LOG_SEVERITY_WARNING:
            return "[WARN] ";
        case LOG_SEVERITY_ERROR:
            return "[ERROR] ";
        case LOG_SEVERITY_CRITICAL:
            return "[FATAL] ";
    }
    return "[UNKNOWN] ";
}

static inline std::tm log_localtime() {
    using std::chrono::system_clock;

    const auto now = system_clock::now();
    const std::time_t datetime = system_clock::to_time_t(now);

    std::tm localtime{};

#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&localtime, &datetime);
#else
    localtime_r(&datetime, &localtime);
#endif

    return localtime;
}

static inline std::string log_date(const std::tm &time) {
    char buffer[32];

    const std::size_t size =
        std::strftime(buffer, sizeof(buffer), "[%Y/%m/%d] ", &time);

    return {buffer, size};
}

static inline std::string log_time(const std::tm &time) {
    char buffer[32];

    const std::size_t size =
        std::strftime(buffer, sizeof(buffer), "[%H:%M:%S] ", &time);

    return {buffer, size};
}

static inline std::string log_datetime(const std::tm &time) {
    std::string out;
    out.resize(128);
    std::size_t size = std::strftime(
        out.data(),
        out.size(),
        "[%Y/%m/%d %H:%M:%S] ",
        &time);
    out.resize(size);
    return out;
}

static inline void push_line(ILogSink &sink, LogSeverity severity, const std::tm &time, std::string_view message, bool flush = false) {
    const auto feature = sink.feature();

    std::string formatted;
    formatted.reserve(message.size() + (32 * feature.size()));

    for (const LogFeature feat : feature) {
        switch (feat) {
            case LogFeature::SEVERITY:
                formatted += log_prefix(severity);
                break;
            case LogFeature::DATETIME:
                formatted += log_datetime(time);
                break;
            case LogFeature::DATE:
                formatted += log_date(time);
                break;
            case LogFeature::TIME:
                formatted += log_time(time);
                break;
            case LogFeature::ARROW:
                formatted += "-> ";
        }
    }
    formatted += message;

    sink.push_line(severity, formatted);
    if (flush) {
        sink.flush();
    }
}

void Logger::flush() {
    if (m_sink_override) {
        m_sink_override->flush();
    }

    for (auto &sink : m_sinks) {
        sink->flush();
    }
}

void Logger::log(LogSeverity severity, std::string_view message) {
    auto lock = std::scoped_lock(s_mutex);

    if (severity < m_severity) {
        return;
    }

    const bool should_flush =
        (severity >= LOG_SEVERITY_CRITICAL);

    const std::tm time = log_localtime();

    if (m_sink_override) {
        push_line(
            *m_sink_override,
            severity, time, message,
            should_flush);
    }

    for (auto &sink : m_sinks) {
        push_line(
            *sink,
            severity, time, message,
            should_flush);
    }
}

class Default_LogSink final : public ILogSink {
    void push_line([[maybe_unused]] LogSeverity severity, std::string_view string) override {
        std::cout << string << '\n';
    }

    void flush() override {
        std::cout.flush();
    }
};

Logger::Logger() : m_sink_override(new Default_LogSink) {};

void Logger::reset_sink_override() {
    auto lock = std::scoped_lock(s_mutex);
    m_sink_override = std::make_unique<Default_LogSink>();
}
