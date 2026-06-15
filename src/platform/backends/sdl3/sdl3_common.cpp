#include "sdl3_common.h"

#include "core/logger.h"

#include <chrono>
#include <ctime>
#include <fstream>

#if USE_THREADS
#include <mutex>
static std::mutex s_mutex;
#endif

static std::unique_ptr<std::ofstream> s_file_sink;

static const char *log_prefix(LogSeverity severity) {
    switch (severity) {
        case LOG_SEVERITY_TRACE:
            return "[TRACE] ";
        case LOG_SEVERITY_INFO:
            return "[INFO] ";
        case LOG_SEVERITY_WARNING:
            return "[WARNING] ";
        case LOG_SEVERITY_ERROR:
            return "[ERROR] ";
        case LOG_SEVERITY_CRITICAL:
            return "[FATAL] ";
    }
    return "";
}

std::string log_datetime() {
    using system_clock = std::chrono::system_clock;
    system_clock::time_point now = system_clock::now();
    std::time_t datetime = system_clock::to_time_t(now);
    std::tm localtime;

#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&localtime, &datetime);
#else
    localtime_r(&datetime, &localtime);
#endif

    std::string out;
    out.resize(128);
    std::size_t size = std::strftime(
        out.data(),
        out.size(),
        "[%Y/%m/%d %H:%M:%S] ",
        &localtime);
    out.resize(size);
    return out;
}

static std::string format_message(LogSeverity severity, std::string_view message) {
    std::string str;
    str.reserve(128);
    str += log_datetime();
    str += log_prefix(severity);
    str += "-> ";
    str += message;
    return str;
}

static void log_callback(LogSeverity severity, std::string_view message) {
    std::string str = format_message(severity, message);
#if USE_THREADS
    std::lock_guard lock{s_mutex};
#endif

#if !defined(EMSCRIPTEN)
    // Lazy load file sink
    if (!s_file_sink) {
        s_file_sink = std::make_unique<std::ofstream>("runtime.log");
        *s_file_sink << "This file was auto-generated at runtime.\n\n";
        s_file_sink->flush();
    }

    SDL_Log("%s\n", str.c_str());
#else
    std::cout << str << '\n';
#endif

    if (s_file_sink) {
        *s_file_sink << str << '\n';
        if (severity == LogSeverity::LOG_SEVERITY_CRITICAL) {
            /* ensures that assertion/crash messages gets
             * displayed on files */
            s_file_sink->flush();
        }
    }
}

void log_init() {
    Logger::get().set_log_callback(log_callback);
}
