#pragma once

#include "application_handler.h"

#include "display_server.h"
#include "event_server.h"
#include "io_server.h"
#include "os_server.h"

class PlatformImpl;
struct PlatformImplState;

/* This class is generic */
class Platform {
public:
    OSServer &os() {
        return this->m_os_server;
    }

    IOServer &io() {
        return this->m_io_server;
    }

    DisplayServer &display() {
        return this->m_display_server;
    }

    EventServer &event() {
        return this->m_event_server;
    }

protected:
    void init();
    void close();
    void update(float ts);
    void render();

    template <typename T, typename... Args>
    void submit_event(Args &&...args) {
        m_event_server.submit<T>(std::forward<Args>(args)...);
    }

private:
    OSServer m_os_server;
    IOServer m_io_server;
    DisplayServer m_display_server;
    EventServer m_event_server;

    ApplicationHandler m_app_handler;

    explicit Platform(PlatformImplState &state)
        : m_os_server{state},
          m_io_server{state},
          m_display_server{state},
          m_app_handler{*this} {}

    friend PlatformImpl;
};
