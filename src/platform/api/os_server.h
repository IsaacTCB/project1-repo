#pragma once

#include "server_macros.inl"

#include <cstdint>

class OSServer {
	DECLARE_SERVER(OSServer);
public:
	[[nodiscard]] const char *get_os() const;
	[[nodiscard]] const char *get_platform() const;

	[[nodiscard]] float get_ticks_sec() const;
	[[nodiscard]] uint64_t get_ticks_ms() const;
};

#undef DECLARE_SERVER
