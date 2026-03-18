#pragma once

#include <memory>

#include "os_server.h"
#include "io_server.h"
#include "display_server.h"

class PlatformImpl;
struct PlatformImplState;

/* This class is generic */
class Platform {
public:
	OSServer& os() {
		return this->os_server;
	}

	IOServer& io() {
		return this->io_server;
	}

	DisplayServer& display() {
		return this->display_server;
	}

private:
	OSServer os_server;
	IOServer io_server;
	DisplayServer display_server;

	explicit Platform(PlatformImplState &state)
		:os_server{state}, io_server{state}, display_server{state}
	{}

	friend PlatformImpl;
};