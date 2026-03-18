#include "../platform.h"

#include "../common/logger.h"

#include "dummy_main.h"
#include "dummy_common.h"

#include "../common/test.h"

#include <iostream>

static bool s_running;

void PlatformImpl::init() {
	Test::run_platform_test(m_platform);
}

void PlatformImpl::iterate() {
	m_platform.io().open_file();
}

void PlatformImpl::close() {
	Logger::close();
}

/* main function */
int main(int argc, char* argv[]) {
	s_running = true;

	PlatformImpl impl{};
	impl.init();
	impl.iterate();
	impl.iterate();
	impl.iterate();
	impl.close();
}