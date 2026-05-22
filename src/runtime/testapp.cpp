#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <numbers>

#include "core/camera.h"
#include "core/events/input_events.h"
#include "core/events/platform_events.h"
#include "core/events/window_events.h"
#include "core/input/gamepad.h"
#include "core/logger.h"
#include "core/view_context_2d.h"
#include "core/viewport.h"

#include "platform/api/application.h"
#include "platform/api/platform.h"

#include "renderer/api/render_queue.h"
#include "renderer/api/renderer.h"

#include "camera_2d_utils.h"
#include "image/decoder.h"
#include "input_service.h"

class TestApplication : public IApplication {
    DECLARE_APP(TestApplication);

public:
    void on_init() override {
        auto &os = m_platform->os();
        LOG_INFO("platform: {}", os.get_platform());
        LOG_INFO("os: {}", os.get_os());

        /* =================== */
        /*       DISPLAY       */
        /* =================== */
        auto &display = m_platform->display();

        /* create main window/screen */
        DisplayWindowProps props{};
        props.width = 800;
        props.height = 600;
        props.resizable = true;
        props.api = RendererAPI::OpenGL;

        m_window = display.init_display(props);
        if (!m_window) {
            LOG_CRITICAL("run_platform_test(): Failed to create main window");
            exit(-1);
        }

        /* initialize renderer */
        m_renderer = Renderer::create(m_window);
        if (!m_renderer) {
            LOG_CRITICAL("run_platform_test(): Failed to create renderer");
            exit(-1);
        }

        ImageData image = Image::decode("test.png");
        if (!image) {
            LOG_CRITICAL("No 'test.png' image file found in the current working directory.");
            exit(-1);
        }

        m_texture = m_renderer->load_texture(image);

        /* poll window size */
        auto [width, height] = display.get_size(m_window);
        LOG_INFO("display size: {0}x{1}", width, height);

        m_input_service.init();

        m_camera.zoom = 0.8f;
        m_camera.anchor = Camera2D::AnchorMode::CENTER;
    }

    void on_update(float delta) override {
        /* clamps deltatime to avoid huge skips */
        delta = std::min(delta, 0.25f);

        /* close application if requested */
        for ([[maybe_unused]] const auto &event : m_platform->event().span<EventPlatformExited>()) {
            m_platform->os().exit_app();
            return;
        }

        for (const auto &event : m_platform->event().span<EventWindow>()) {
            if (const auto *ptr = std::get_if<EventWindowResized>(&event)) {
                LOG_TRACE("{0}x{1}", ptr->width, ptr->height);
                m_renderer->on_screen_resized(ptr->width, ptr->height);
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

        if (m_input_service.is_key_just_pressed(Input::Key::Scancode::SPACE)) {
            LOG_TRACE("Damn");
        }

        if (m_input_service.is_key_just_released(Input::Key::Scancode::SPACE)) {
            LOG_TRACE("Undamn");
        }

        if (m_input_service.is_mouse_button_just_pressed(Input::Mouse::Buttons::LEFT)) {
            LOG_TRACE("Walk");
        }
        if (m_input_service.is_mouse_button_just_released(Input::Mouse::Buttons::RIGHT)) {
            LOG_TRACE("Walked");
        }

        float move_x = 0.0f, move_y = 0.0f;
        m_input_service.pad_foreach([&](Input::Gamepad::DeviceID gamepad) {
            if (m_input_service.is_pad_button_just_pressed(gamepad, Input::Gamepad::Buttons::SOUTH)) {
                LOG_TRACE("Pad {0} just pressed X.", gamepad);
            } else if (m_input_service.is_pad_button_just_released(gamepad, Input::Gamepad::Buttons::SOUTH)) {
                LOG_TRACE("Pad {0} just released X.", gamepad);
            }

            auto [axis_x, axis_y] = m_input_service.get_pad_stick(gamepad, false);
            move_x = axis_x;
            move_y = axis_y;
        });

        constexpr float PLAYER_SPEED = 60 * 3.0f;
        if (m_input_service.is_key_pressed(Input::Key::Scancode::A)) {
            move_x = -1.0f;
        } else if (m_input_service.is_key_pressed(Input::Key::Scancode::D)) {
            move_x = 1.0f;
        }

        if (m_input_service.is_key_pressed(Input::Key::Scancode::W)) {
            move_y = -1.0f;
        } else if (m_input_service.is_key_pressed(Input::Key::Scancode::S)) {
            move_y = 1.0f;
        }

        if (m_input_service.is_key_pressed(Input::Key::Scancode::Q)) {
            m_camera.rotation -= 0.6 * std::numbers::pi * delta;
        } else if (m_input_service.is_key_pressed(Input::Key::Scancode::E)) {
            m_camera.rotation += 0.6 * std::numbers::pi * delta;
        }

        if (m_input_service.is_key_pressed(Input::Key::Scancode::KP_PLUS)) {
            m_camera.zoom += 1.0f * delta;
        } else if (m_input_service.is_key_pressed(Input::Key::Scancode::KP_MINUS)) {
            m_camera.zoom -= 1.0f * delta;
        }

        m_camera.zoom += 0.1f * m_input_service.get_mouse_wheel_y();
        m_camera.zoom = std::min(m_camera.zoom, 2.0f);
        m_camera.zoom = std::max(m_camera.zoom, 0.1f);

        float length = std::sqrt((move_x * move_x) + (move_y * move_y));
        if (length > 1.0f) {
            move_x /= length;
            move_y /= length;
        }

        float cam_cos = std::cosf(m_camera.rotation);
        float cam_sin = std::sinf(m_camera.rotation);
        m_entity.x += ((move_x * cam_cos) - (cam_sin * move_y)) * (PLAYER_SPEED * delta);
        m_entity.y += ((move_x * cam_sin) + (cam_cos * move_y)) * (PLAYER_SPEED * delta);

        m_camera.x = std::floor(m_entity.x + 100.0f);
        m_camera.y = std::floor(m_entity.y + 100.0f);
    }

    void on_render() override {
        m_queue.clear();

        /* TODO: verify if texture actually exists. */
        m_queue.draw_color(RGB8(88, 90, 220), 0, 0, 400, 400);
        m_queue.draw_color(RGBA8(48, 45, 120, 80), 40, 40, 320, 320);
        m_queue.draw_texture(m_texture, std::floor(m_entity.x), std::floor(m_entity.y), 200, 200);

        auto [mouse_x, mouse_y] = m_input_service.get_mouse_pos();

        constexpr std::array<Viewport, 4> viewports{{
            {.x = 0.0f, .y = 0.0f, .width = 0.5f, .height = 0.5f},
            {.x = 0.5f, .y = 0.0f, .width = 0.5f, .height = 0.5f},
            {.x = 0.0f, .y = 0.5f, .width = 0.5f, .height = 0.5f},
            {.x = 0.5f, .y = 0.5f, .width = 0.5f, .height = 0.5f},
        }};

        m_renderer->clear(0.1f, 0.1f, 0.1f, 0.0f);
        for (const auto &viewport : viewports) {
            Viewport abs_viewport = m_renderer->get_absolute_viewport(viewport);

            ViewContext2D ctx = Camera2DUtils::build_view_context(m_camera, abs_viewport);

            auto [world_x, world_y] = ctx.screen_to_world(mouse_x, mouse_y);

            auto [mx, my] = ctx.world_to_screen(world_x, world_y);
            auto [world2_x, world2_y] = ctx.screen_to_world(mx, my);
            m_queue.draw_color(0xFF000060, world2_x - 4, world2_y - 4, 16, 16);
            m_queue.draw_color(0xFF00FFFF, world_x, world_y, 8, 8);

            auto bounds = ctx.get_bounds_aabb();
            m_queue.draw_color(
                0xFF00FF20,
                bounds.left + 8,
                bounds.top + 8,
                bounds.width() - 16,
                bounds.height() - 16);

            m_renderer->render_pass(ctx, m_queue);
        }
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
    Camera2D m_camera;
};

SET_APP(TestApplication)
