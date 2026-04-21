#include "platform.h"

void Platform::init() {
    m_app_handler.init();
}

void Platform::close() {
    m_app_handler.close();
}

void Platform::update(float ts) {
    m_app_handler.update(ts);
}

void Platform::render() {
    m_app_handler.render();
}

void Platform::send_event() {
}
