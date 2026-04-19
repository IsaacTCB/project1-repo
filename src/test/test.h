#pragma once

#include "../platform/api/platform.h"

#include "application.h"

class ApplicationHandler {
public:
    ApplicationHandler(Platform &platform);
    ~ApplicationHandler() = default;

    void init();
    void update(float ts);
    void render();
    void close();

private:
    Platform &m_platform;
};

