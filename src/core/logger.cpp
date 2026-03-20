#include "logger.h"

#include <iostream>

std::unique_ptr<Logger> Logger::s_logger;

Logger& Logger::get() {
    if (!s_logger) {
        s_logger.reset(new Logger{});
    }
    return *s_logger;
}

void Logger::close() {
    s_logger.reset();
}

void Logger::log(LogSeverity severity, std::string_view message) {
    if (severity < m_severity) {
        return;
    }
    if (m_callback) {
        m_callback(severity, message);
    }
}

static void default_log_callback(LogSeverity severity, std::string_view message) {
    const char *prefix = nullptr;
    switch (severity) {
        case LOG_SEVERITY_TRACE:
            prefix = "[TRACE] ";
            break;
        case LOG_SEVERITY_INFO:
            prefix = "[INFO] ";
            break;
        case LOG_SEVERITY_WARNING:
            prefix = "[WARNING] ";
            break;
        case LOG_SEVERITY_ERROR:
            prefix = "[ERROR] ";
            break;
        case LOG_SEVERITY_CRITICAL:
            prefix = "[FATAL] ";
            break;
    }

    if (prefix) {
        std::cout << prefix << message << std::endl;
    } else {
        std::cout << message << std::endl;
    }
}

Logger::Logger() :m_callback(default_log_callback) {}

