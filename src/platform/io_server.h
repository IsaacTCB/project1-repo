#pragma once

#include "common/server_macros.inl"

class IOServer {
	DECLARE_SERVER(IOServer);
public:
	void open_file();
};

#undef DECLARE_SERVER