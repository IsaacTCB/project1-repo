#include "application_handler.h"

#include "application.h"
#include "core/assertion.h"
#include "core/logger.h"
#include "runtime/platform_services.h"

#include <cstdlib>

static std::unique_ptr<IApplication> m_app;

void ApplicationHandler::init(const PlatformServices &platform_services) {
    m_app = IApplication::create_application(platform_services);
    ASSERT(m_app, "Unable to create application");

    if (!m_app) {
        /* Quit application on fail
         * TODO: use platform layer for this instead
         */
        LOG_CRITICAL("Failed to initialize application!");
        exit(-10);
    }

    m_app->on_init();
}

void ApplicationHandler::update(float delta) {
    ASSERT(m_app, "Application does not exist!");
    m_app->on_update(delta);
}

void ApplicationHandler::render() {
    ASSERT(m_app, "Application does not exist!");
    m_app->on_render();
}

void ApplicationHandler::close() {
    ASSERT(m_app, "Application does not exist!");
    m_app->on_close();
    m_app.reset(nullptr);
}
