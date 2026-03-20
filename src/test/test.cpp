#include "test.h"

#include "platform/platform.h"

#include "renderer/renderer.h"
#include "renderer/renderer_apis.h"

#include "core/logger.h"

#include <cstdlib>
#include <memory>

static DisplayWindowHandle s_window;
static std::shared_ptr<Renderer> s_renderer;

void Test::run_platform_test(Platform& platform) {
    LOG_TRACE("run_platform_test(): Ensure all platforms behave similarly.");

    auto& os = platform.os();
    LOG_INFO("platform: {}", os.get_platform());
    LOG_INFO("os: {}", os.get_os());

    /* =================== */
    /*       DISPLAY       */
    /* =================== */
    auto& display = platform.display();

    /* create main window/screen */
    DisplayWindowProps props{};
    props.width = 800;
    props.height = 600;
    props.api = RendererAPI::SDL3Renderer;

    s_window = display.init_display(props);
    if (!s_window) {
        LOG_CRITICAL("run_platform_test(): Failed to create main window");
        exit(0);
    }

    /* initialize renderer */
    s_renderer = Renderer::create(s_window);
    if (!s_renderer) {
        LOG_CRITICAL("run_platform_test(): Failed to create renderer");
        exit(0);
    }

    /* poll window size */
    auto [width, height] = display.get_size(s_window);
    LOG_INFO("display size: {0}x{1}", width, height);
}

void Test::run_platform_render_test(Platform& platform) {
    s_renderer->clear(0.1f, 0.1f, 0.1f, 0.0f);
    s_renderer->swap_buffers();
}

void Test::run_platform_stop_test(Platform& platform) {
    s_renderer.reset();
    platform.display().close_display(s_window);
}
