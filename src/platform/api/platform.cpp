#include "platform.h"

void Platform::init() {
    app_handler.init();
}

void Platform::close() {
    app_handler.close();
}

void Platform::update(float ts) {
    app_handler.update(ts);
}

void Platform::render() {
    app_handler.render();
}

void Platform::send_event() {
}
