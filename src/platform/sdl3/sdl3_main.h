#pragma once

#include "../platform.h"

#include "sdl3_common.h"

class PlatformImpl {
public:
	PlatformImpl()
		:m_state{}, m_platform{m_state}
	{}

	SDL_AppResult init();
	SDL_AppResult iterate();
	SDL_AppResult event(const SDL_Event &event);
	void close(SDL_AppResult result);

	bool is_running() const noexcept { return m_state.running; }

private:
	PlatformImplState m_state;
	Platform m_platform;
};
