#include "application.h"

#include <memory>

#include "core/logger.h"

#include "platform/api/platform.h"
#include "renderer/api/renderer.h"

class TestApplication : public IApplication {
    DECLARE_APP(TestApplication);
public:
    void on_init() override {
        auto& os = m_platform->os();
        LOG_INFO("m_platform: {}", os.get_platform());
        LOG_INFO("os: {}", os.get_os());

        /* =================== */
        /*       DISPLAY       */
        /* =================== */
        auto& display = m_platform->display();

        /* create main window/screen */
        DisplayWindowProps props{};
        props.width = 800;
        props.height = 600;
        props.api = RendererAPI::SDL3Renderer;

        m_window = display.init_display(props);
        if (!m_window) {
            LOG_CRITICAL("run_platform_test(): Failed to create main window");
            exit(0);
        }

        /* initialize renderer */
        m_renderer = Renderer::create(m_window);
        if (!m_renderer) {
            LOG_CRITICAL("run_platform_test(): Failed to create renderer");
            exit(0);
        }

        /* poll window size */
        auto [width, height] = display.get_size(m_window);
        LOG_INFO("display size: {0}x{1}", width, height);
    }

    void on_update(float delta) override {}

    void on_render() override {
        m_renderer->clear(0.1f, 0.1f, 0.1f, 0.0f);
        m_renderer->swap_buffers();
    }

    void on_close() override {
        m_platform->display().close_display(m_window);
        m_renderer.reset();
    }
private:
    DisplayWindowHandle m_window;
    std::shared_ptr<Renderer> m_renderer;
};

SET_APP(TestApplication)
