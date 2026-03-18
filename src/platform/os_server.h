#pragma once

#include "common/server_macros.inl"

class OSServer {
	DECLARE_SERVER(OSServer);
public:
	const char *get_os() const;
	const char *get_platform() const;
};

#undef DECLARE_SERVER