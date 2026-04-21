#pragma once

class Platform;

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
