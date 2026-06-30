#include "application_handler.h"

#include "application.h"
#include "core/assertion.h"
#include "runtime/platform_services.h"

#include <cstdlib>

void IApplicationHandler::init(const IApplication::Servers &app_servers, const PlatformServices &platform_services) {
    m_app = std::unique_ptr<IApplication>(create_app(app_servers, platform_services));
    ASSERT(m_app, "Unable to create application");

    if (!m_app) {
        exit(-1);
    }

    m_app->on_init();
}

void IApplicationHandler::update(float delta) {
    ASSERT(m_app, "Application does not exist!");
    m_app->on_update(delta);
}

void IApplicationHandler::render(RenderQueue &queue) {
    ASSERT(m_app, "Application does not exist!");
    m_app->on_draw(queue);
}

void IApplicationHandler::close() {
    ASSERT(m_app, "Application does not exist!");
    m_app->on_close();
    m_app.reset(nullptr);
}
