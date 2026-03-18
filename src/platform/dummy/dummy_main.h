#pragma once

#include "../platform.h"

#include "dummy_common.h"

class PlatformImpl {
public:
	PlatformImpl()
		:m_platform(m_state)
	{}

	void init();
	void iterate();
	void close();

private:
	PlatformImplState m_state;
	Platform m_platform;
};