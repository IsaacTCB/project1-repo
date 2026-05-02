#include <memory>

#include "core/events/input_events.h"
#include "core/events/window_events.h"
#include "core/image/decoder.h"
#include "core/keys/gamepad.h"
#include "core/logger.h"

#include "platform/api/application.h"
#include "platform/api/platform.h"

#include "renderer/api/render_queue.h"
#include "renderer/api/renderer.h"

#include "test/input_service.h"

class TestApplication : public IApplication {
    DECLARE_APP(TestApplication);

public:
    void on_init() override {
        auto &os = m_platform->os();
        LOG_INFO("m_platform: {}", os.get_platform());
        LOG_INFO("os: {}", os.get_os());

        /* =================== */
        /*       DISPLAY       */
        /* =================== */
        auto &display = m_platform->display();

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

        m_input_service.init();
    }

    void on_update(float delta) override {
        // m_input_service.update(delta);
        for (const auto &event : m_platform->event().span<EventWindow>()) {
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

        m_input_service.update();

        for (const auto &event : m_platform->event().span<EventInput>()) {
            m_input_service.event(event);
        }

        if (m_input_service.is_key_just_pressed(Key::Scancode::SPACE)) {
            LOG_TRACE("Damn");
        }

        if (m_input_service.is_key_just_released(Key::Scancode::SPACE)) {
            LOG_TRACE("Undamn");
        }

        constexpr float PLAYER_SPEED = 3.0f;
        if (m_input_service.is_key_pressed(Key::Scancode::A)) {
            m_entity.x -= PLAYER_SPEED;
        } else if (m_input_service.is_key_pressed(Key::Scancode::D)) {
            m_entity.x += PLAYER_SPEED;
        }

        if (m_input_service.is_key_pressed(Key::Scancode::W)) {
            m_entity.y -= PLAYER_SPEED;
        } else if (m_input_service.is_key_pressed(Key::Scancode::S)) {
            m_entity.y += PLAYER_SPEED;
        }

        if (m_input_service.is_mouse_button_just_pressed(Key::MouseButtons::LEFT)) {
            LOG_TRACE("Walk");
        }
        if (m_input_service.is_mouse_button_just_released(Key::MouseButtons::RIGHT)) {
            LOG_TRACE("Walked");
        }

        m_input_service.pad_foreach([&](Gamepad::DeviceID gamepad) {
            if (m_input_service.is_pad_button_just_pressed(gamepad, Gamepad::Buttons::SOUTH)) {
                LOG_TRACE("Pad {0} just pressed X.", gamepad);
            } else if (m_input_service.is_pad_button_just_released(gamepad, Gamepad::Buttons::SOUTH)) {
                LOG_TRACE("Pad {0} just released X.", gamepad);
            }
        });
    }

    void on_render() override {
        m_queue.clear();

        /* TODO: verify if texture actually exists. */
        m_queue.draw_color(RGB8(88, 90, 220), 0, 0, 400, 400);
        m_queue.draw_color(RGBA8(48, 45, 120, 80), 40, 40, 320, 320);
        m_queue.draw_texture(m_texture, m_entity.x, m_entity.y, 200, 200);

        auto [mouse_x, mouse_y] = m_input_service.get_mouse_pos();
        m_queue.draw_color(0xFF00FFFF, mouse_x, mouse_y, 8, 8);

        m_renderer->clear(0.1f, 0.1f, 0.1f, 0.0f);
        m_renderer->render_pass(m_queue);
        m_renderer->swap_buffers();
    }

    void on_close() override {
        m_input_service.close();

        m_renderer->destroy_texture(m_texture);
        m_renderer.reset();

        m_platform->display().close_display(m_window);
    }

private:
    struct Entity {
        float x = 0.0f;
        float y = 0.0f;
    };

    Entity m_entity;
    DisplayWindowHandle m_window;
    std::unique_ptr<Renderer> m_renderer;

    TextureHandle m_texture;
    RenderQueue m_queue;

    InputService m_input_service;
};

SET_APP(TestApplication)
