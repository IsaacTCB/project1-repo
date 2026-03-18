#pragma once

#include "../platform.h"
#include "sdl3_common.h"

class PlatformImpl {
public:
	PlatformImpl()
		:m_platform(m_state)
	{
	}

	void init();
	void iterate();
	void event(const SDL_Event &event);
	void close();

	bool is_running() const noexcept { return m_state.running; }

private:
	PlatformImplState m_state;
	Platform m_platform;
};