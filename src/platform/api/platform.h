#pragma once

#include "os_server.h"
#include "io_server.h"
#include "display_server.h"
#include "application_handler.h"

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

protected:
	void init();
	void close();
	void update(float ts);
	void render();
	void send_event(); /* to be implemented */

private:
	OSServer os_server;
	IOServer io_server;
	DisplayServer display_server;
	ApplicationHandler app_handler;

	explicit Platform(PlatformImplState &state)
		:os_server{state},
        io_server{state},
        display_server{state},
		app_handler{*this} {}

	friend PlatformImpl;
};
