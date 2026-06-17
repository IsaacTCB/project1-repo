#pragma once

#include <cstdint>

#include "platform/api/platform.h"
#include "psp_common.h"

enum class PlatformAppState : uint8_t {
    CONTINUE,
    QUIT,
};

class PlatformImpl {
public:
    PlatformImpl()
        : m_state{}, m_platform{m_state} {}

    PlatformAppState init(int argc, char *argv[]);
    PlatformAppState iterate();
    void close(PlatformAppState app_state);

public:
    static int exit_callback(int arg1, int arg2, void *common);

private:
    void poll_inputs();

private:
    PlatformImplState m_state;
    Platform m_platform;
};
