#pragma once

#include <cstdint>
#include <format>
#include <memory>
#include <string_view>

enum LogSeverity : uint8_t {
    LOG_SEVERITY_TRACE = 0,
    LOG_SEVERITY_INFO,
    LOG_SEVERITY_WARNING,
    LOG_SEVERITY_ERROR,
    LOG_SEVERITY_CRITICAL,
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
    static void close();

public:
    Logger(const Logger &logger) = delete;
    ~Logger() = default;
    Logger(Logger &&logger) = delete;
    Logger &operator=(Logger &other) = delete;
    Logger &operator=(Logger &&other) = delete;

    /**
     * @brief Override log callback
     *
     * @param callback A callback function that prints onto something.
     */
    void set_log_callback(LogFn callback) noexcept {
        m_callback = callback;
    }

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

private:
    Logger();

    void log(LogSeverity severity, std::string_view message);

private:
    LogFn m_callback;
    LogSeverity m_severity = LOG_SEVERITY_TRACE;

    static std::unique_ptr<Logger> s_logger;
};

#define LOG_TRACE(...) ::Logger::get().print(LOG_SEVERITY_TRACE, __VA_ARGS__)
#define LOG_INFO(...) ::Logger::get().print(LOG_SEVERITY_INFO, __VA_ARGS__)
#define LOG_WARN(...) ::Logger::get().print(LOG_SEVERITY_WARNING, __VA_ARGS__)
#define LOG_ERROR(...) ::Logger::get().print(LOG_SEVERITY_ERROR, __VA_ARGS__)
#define LOG_CRITICAL(...) ::Logger::get().print(LOG_SEVERITY_CRITICAL, __VA_ARGS__)
