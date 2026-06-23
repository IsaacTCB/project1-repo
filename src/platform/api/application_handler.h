#pragma once

#include "runtime/platform_services.h"

class ApplicationHandler {
public:
    void init(const PlatformServices &platform_services);
    void update(float delta);
    void render();
    void close();
};
