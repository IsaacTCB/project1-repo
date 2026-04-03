#pragma once

#include "server_macros.inl"

class OSServer {
	DECLARE_SERVER(OSServer);
public:
	[[nodiscard]] const char *get_os() const;
	[[nodiscard]] const char *get_platform() const;
};

#undef DECLARE_SERVER
