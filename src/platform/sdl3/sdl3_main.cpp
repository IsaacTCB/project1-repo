#include <SDL3/SDL.h>

#include "sdl3_common.h"
#include "sdl3_main.h"

#include "../../common/logger.h"
#include "../../test/test.h"

int main(int argc, char* argv[]) {
    log_init();

    PlatformImpl impl{};
    impl.init();
    while (impl.is_running()) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            impl.event(event);
        }
        impl.iterate();
    }
    impl.close();
}

void PlatformImpl::init() {
    if (!SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO)) {
        LOG_CRITICAL("Failed to initialize SDL3");
        m_state.running = false;
        return;
    }

    m_state.running = true;
    Test::run_platform_test(m_platform);
}

void PlatformImpl::iterate() {
    Test::run_platform_render_test(m_platform);
}

void PlatformImpl::event(const SDL_Event &event) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
        m_state.running = false;
        break;
    }
}

void PlatformImpl::close() {
    Test::run_platform_stop_test(m_platform);
    m_state.running = false;
}
