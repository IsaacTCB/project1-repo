#pragma once

#include <cstdint>
#include <format>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

enum LogSeverity : uint8_t {
    LOG_SEVERITY_TRACE = 0,
    LOG_SEVERITY_INFO,
    LOG_SEVERITY_WARNING,
    LOG_SEVERITY_ERROR,
    LOG_SEVERITY_CRITICAL,
};

enum class LogFeature : uint8_t {
    SEVERITY,
    DATETIME,
    DATE,
    TIME,
    ARROW,
};

class ILogSink {
public:
    [[nodiscard]]
    virtual std::span<const LogFeature> feature() const {
        static constexpr LogFeature features[] = {
            LogFeature::SEVERITY,
        };
        return features;
    }

    virtual ~ILogSink() = default;
    virtual void push_line(LogSeverity severity, std::string_view string) = 0;
    virtual void flush() = 0;
};

/**
 * @class Logger
 * @brief A global singleton for printing stuff.
 *
 * It is highly recommended that the platform overrides
 * the default callback behavior with `set_log_callback()`.
 *
 * @usage Logger::get().print("hello")
 */
class Logger {
public:
    using LogFn = void (*)(LogSeverity severity, std::string_view message);

    [[nodiscard]] static Logger &get();

public:
    Logger(const Logger &logger) = delete;
    ~Logger() = default;
    Logger(Logger &&logger) = delete;
    Logger &operator=(Logger &other) = delete;
    Logger &operator=(Logger &&other) = delete;

    /**
     * @brief Adds a new sink into the logger
     *
     * The logger *owns* the sink.
     *
     * @param sink A valid sink.
     */
    void add_sink(std::unique_ptr<ILogSink> sink);

    /**
     * @brief Overrides the default sink.
     *
     * Typically used to setup the main platform console sink.
     *
     * @param sink Sink to be applied
     */
    void set_sink_override(std::unique_ptr<ILogSink> sink);

    /**
     * @brief Resets the current sink override to the default one.
     */
    void reset_sink_override();

    /**
     * @brief Set the minimum severity needed for logging.
     *
     * @param severity Must be >= 0 && < LOGSEVERITY_MAX
     */
    void set_severity(LogSeverity severity) noexcept {
        m_severity = severity;
    }

    /**
     * @brief Prints text onto the callback sink
     *
     * @param severity The severity of this message
     * (LOG_SEVERITY_TRACE, LOG_SEVERITY_INFO,
     * LOG_SEVERITY_ERROR, etc...)
     * @param message The string to print
     */
    void print(LogSeverity severity, std::string_view message) {
        log(severity, message);
    }

    /**
     * @brief Formats text and prints it onto the callback sink
     *
     * Uses std::format internally.
     *
     * @param severity The severity of this message
     * (LOG_SEVERITY_TRACE, LOG_SEVERITY_INFO,
     * LOG_SEVERITY_ERROR, etc...)
     * @param fmt The format string
     * @param args... The parameters for the format string
     */
    template <typename... Args>
    void print(LogSeverity severity, std::format_string<Args...> fmt, Args &&...args) {
        std::string formatted = std::format(fmt,
            std::forward<Args>(args)...);
        log(severity, formatted);
    }

    /**
     * @brief Prints object onto the callback sink
     *
     * This is a helper function for printing without having
     * to specify a format string earlier.
     * Uses std::format internally.
     *
     * @param severity The severity of this message
     * (LOG_SEVERITY_TRACE, LOG_SEVERITY_INFO,
     * LOG_SEVERITY_ERROR, etc...)
     * @param fmt The format string
     * @param args... The parameters for the format string
     */
    template <typename T>
    void print(LogSeverity severity, T &&value) {
        print(severity, "{}", std::forward<T>(value));
    }

    void flush();

private:
    Logger();

    void log(LogSeverity severity, std::string_view message);

private:
    LogSeverity m_severity = LOG_SEVERITY_TRACE;

    std::vector<std::unique_ptr<ILogSink>> m_sinks;
    std::unique_ptr<ILogSink> m_sink_override;
};

#define LOG_TRACE(...) ::Logger::get().print(LOG_SEVERITY_TRACE, __VA_ARGS__)
#define LOG_INFO(...) ::Logger::get().print(LOG_SEVERITY_INFO, __VA_ARGS__)
#define LOG_WARN(...) ::Logger::get().print(LOG_SEVERITY_WARNING, __VA_ARGS__)
#define LOG_ERROR(...) ::Logger::get().print(LOG_SEVERITY_ERROR, __VA_ARGS__)
#define LOG_CRITICAL(...) ::Logger::get().print(LOG_SEVERITY_CRITICAL, __VA_ARGS__)
