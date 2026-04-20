#include "application.h"

#include <cmath>
#include <memory>

#include "core/image/decoder.h"
#include "core/logger.h"

#include "platform/api/platform.h"
#include "renderer/api/renderer.h"
#include "renderer/api/render_queue.h"

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

        ImageData image = Image::decode("test.png");
        if (image) {
            LOG_INFO("Loaded 'test.png'");
            m_texture = m_renderer->load_texture(image);
        }

        /* poll window size */
        auto [width, height] = display.get_size(m_window);
        LOG_INFO("display size: {0}x{1}", width, height);
    }

    void on_update(float delta) override {}

    void on_render() override {
        constexpr float SPEED = 3.0f;
        constexpr float MAGNITUDE = 30.0f;

        float sin = std::sinf(m_platform->os().get_ticks_sec() * SPEED);
        float cos = std::cosf(m_platform->os().get_ticks_sec() * SPEED);

        m_queue.clear();

        /* TODO: verify if texture actually exists. */
        m_queue.draw_color(0xFFFF00FF, 0, 0, 400, 400);
        m_queue.draw_texture(m_texture, 40 + (sin * MAGNITUDE), 38 + (cos * MAGNITUDE), 200, 200);

        m_renderer->clear(0.1f, 0.1f, 0.1f, 0.0f);
        m_renderer->render_pass(m_queue);
        m_renderer->swap_buffers();
    }

    void on_close() override {
        m_renderer->destroy_texture(m_texture);
        m_platform->display().close_display(m_window);
        m_renderer.reset();
    }
private:
    DisplayWindowHandle m_window;
    std::shared_ptr<Renderer> m_renderer;

    TextureHandle m_texture;
    RenderQueue m_queue;
};

SET_APP(TestApplication)
