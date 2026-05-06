#include "platform.h"

void Platform::init() {
    m_app_handler.init();
}

void Platform::close() {
    m_app_handler.close();
}

void Platform::update(float ts) {
    m_app_handler.update(ts);
    m_event_server.m_events.clear();
}

void Platform::render() {
    m_app_handler.render();
}
