#include "platform.h"

#include "core/events/input_events.h"
#include "core/events/window_events.h"
#include "core/logger.h"

void Platform::init() {
    m_app_handler.init();
}

void Platform::close() {
    m_app_handler.close();
}

void Platform::update(float ts) {
    m_app_handler.update(ts);

    for (const auto &event: m_events.span<EventWindow>()) {
        if (const auto *ptr = std::get_if<EventWindowResized>(&event)) {
            LOG_TRACE("{0}x{1}", ptr->width, ptr->height);
        }
        if (std::get_if<EventWindowMaximized>(&event)) {
            LOG_TRACE("maximized");
        }
        if (std::get_if<EventWindowMinimized>(&event)) {
            LOG_TRACE("minimized");
        }
    }

    for (const auto &event: m_events.span<EventInput>()) {
        if (const auto *ptr = std::get_if<EventInputKey>(&event)) {
            LOG_TRACE("{} {}",
                ptr->pressed ? "Pressed" : "Released",
                ptr->key);
        }
        if (const auto *ptr = std::get_if<EventInputMouseMotion>(&event)) {
            LOG_TRACE("{} {}",
                ptr->rel_x,
                ptr->rel_y);
        }
    }

    m_events.clear();
}

void Platform::render() {
    m_app_handler.render();
}
