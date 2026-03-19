#include "dummy_common.h"

#include "../core/Logger.h"

#include <chrono>
#include <ctime>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>

static std::unique_ptr<std::ofstream> s_file_sink;
static std::mutex s_mutex;

static const char* log_prefix(LogSeverity severity) {
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

std::string log_test() {
	std::string out;
	out.resize(128);

	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::time_t datetime = std::chrono::system_clock::to_time_t(now);

	std::tm localtime;
	// FIXME: this is windows-only
	localtime_s(&localtime, &datetime);

	std::size_t size = std::strftime(out.data(), out.size(), "[%Y/%m/%d %H:%M:%S] ", &localtime);
	out.resize(size);
	return out;
}

static std::string format_message(LogSeverity severity, std::string_view message) {
	std::string str;
	str.reserve(128);
	str += log_test();
	str += log_prefix(severity);
	str += "-> ";
	str += message;
	return str;
}

void Logger::log(LogSeverity severity, std::string_view message, RendererAPI api) {
	if (severity < m_severity) {
		return;
	}

	std::string str = format_message(severity, message);
	std::lock_guard lock{ s_mutex };

	// Lazy load file sink
	if (!s_file_sink) {
		s_file_sink = std::make_unique<std::ofstream>("runtime.log");
		*s_file_sink << "This file was auto-generated at runtime.\n\n";
		s_file_sink->flush();
	}

	std::cout << str << '\n';
	*s_file_sink << str << '\n';
}
