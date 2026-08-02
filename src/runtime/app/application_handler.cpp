#include "application_handler.h"

#include <cstdlib>

#include "application.h"
#include "core/assertion.h"
#include "core/logger.h"
#include "runtime/app/context/frame_context.h"
#include "runtime/platform_services.h"

void IApplicationHandler::init(const IApplication::Servers &app_servers, const PlatformServices &platform_services) {
    m_app = std::unique_ptr<IApplication>(create_app(app_servers, platform_services));
    ASSERT(m_app, "Unable to create application");

    if (!m_app) {
        exit(-1);
    }

    LOG_TRACE("init application: {}", m_app->get_name());
    m_app->on_init();
}

void IApplicationHandler::update(UpdateContext &ctx) {
    ASSERT(m_app, "Application does not exist!");
    m_app->on_update(ctx);
}

void IApplicationHandler::render(RenderContext &ctx) {
    ASSERT(m_app, "Application does not exist!");
    m_app->on_draw(ctx);
}

void IApplicationHandler::close() {
    ASSERT(m_app, "Application does not exist!");
    m_app->on_close();
    m_app.reset(nullptr);
}
