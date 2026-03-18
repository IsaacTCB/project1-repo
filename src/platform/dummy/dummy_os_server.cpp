#include "../os_server.h"

#include "dummy_common.h"

const char* OSServer::get_os() const {
#if defined(_WIN32) || defined(_WIN64)
    return "Windows";
#elif defined(__APPLE__) || defined(__MACH__)
    return "macOS/iOS";
#elif defined(__linux__)
    return "Linux";
#elif defined(__unix__) || defined(BSD)
    return "Unix/BSD";
#else
    return "Unknown OS";
#endif
}

const char* OSServer::get_platform() const {
	return "dummy";
}