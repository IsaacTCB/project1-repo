#pragma once

#include "server_macros.inl"

class IOServer {
	DECLARE_SERVER(IOServer);
public:
	void open_file();
};

#undef DECLARE_SERVER
