#pragma once

#include <string_view>
#include <format>

enum LogSeverity {
	LOG_SEVERITY_TRACE = 0,
	LOG_SEVERITY_INFO,
	LOG_SEVERITY_WARNING,
	LOG_SEVERITY_ERROR,
	LOG_SEVERITY_CRITICAL,
};

class Logger {
public:
	template <typename ...Args>
	void print(LogSeverity severity, std::string_view message) {
		log(severity, message);
	}

	template <typename First, typename ...Rest>
	void print(LogSeverity severity, std::format_string<First, Rest...> fmt, First &&first, Rest &&...rest) {
		std::string formatted = std::format(fmt,
			std::forward<First>(first),
			std::forward<Rest>(rest)...
		);
		log(severity, formatted);
	}

	void set_severity(LogSeverity severity) {
		m_severity = severity;
	}

	[[nodiscard]]
	static Logger& get() {
		if (!s_Logger) {
			s_Logger = new Logger();
		}
		return *s_Logger;
	}

	static void close() {
		delete s_Logger;
	}

private:
	void log(LogSeverity severity, std::string_view message);

	LogSeverity m_severity = LOG_SEVERITY_TRACE;

private:
	Logger() = default;
	~Logger() = default;

	Logger(const Logger& logger) = delete;
	Logger(Logger&& logger) = delete;
	Logger& operator=(Logger& other) = delete;
	Logger& operator=(Logger&& other) = delete;

	inline static Logger *s_Logger = nullptr;
};

#define LOG_TRACE(...)    ::Logger::get().print(LOG_SEVERITY_TRACE, __VA_ARGS__)
#define LOG_INFO(...)     ::Logger::get().print(LOG_SEVERITY_INFO, __VA_ARGS__)
#define LOG_WARN(...)     ::Logger::get().print(LOG_SEVERITY_WARNING, __VA_ARGS__)
#define LOG_ERROR(...)    ::Logger::get().print(LOG_SEVERITY_ERROR, __VA_ARGS__)
#define LOG_CRITICAL(...) ::Logger::get().print(LOG_SEVERITY_CRITICAL, __VA_ARGS__)
