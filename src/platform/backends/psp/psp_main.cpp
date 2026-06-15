#include "psp_platform.h"

int main(int argc, char *argv[]) {
    PlatformImpl m_platform_impl{};

    PlatformAppState state = m_platform_impl.init(argc, argv);
    while (state == PlatformAppState::CONTINUE) {
        state = m_platform_impl.iterate();
    }
    m_platform_impl.close(state);
    return 0;
}
