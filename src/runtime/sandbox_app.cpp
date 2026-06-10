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
#include "core/math/color.h"
#include "core/math/vec2.h"
#include "core/math/vec2i.h"
#include "core/view_context_2d.h"
#include "core/viewport.h"

#include "platform/api/application.h"
#include "platform/api/platform.h"

#include "renderer/api/render_handles.h"
#include "renderer/api/render_queue.h"
#include "renderer/api/renderer.h"

#include "asset_service.h"
#include "camera_2d_utils.h"
#include "input_service.h"

class SandboxApplication : public IApplication {
    DECLARE_APP(SandboxApplication);

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
        props.size = {800, 600};
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

        /* initialize asset service */
        m_asset_service.emplace(::AssetService::Provider{
            .renderer{
                .load_texture = [](ImageData &image, void *userdata) -> TextureHandle {
                    ASSERT(userdata);
                    auto &renderer = *static_cast<std::unique_ptr<Renderer> *>(userdata);
                    return renderer->load_texture(image);
                },
                .unload_texture = [](TextureHandle texture, void *userdata) {
                    ASSERT(userdata);
                    auto &renderer = *static_cast<std::unique_ptr<Renderer> *>(userdata);
                    renderer->destroy_texture(texture);
                    // this comment fixes clang-format. do not remove it
                },
                .userdata = &m_renderer,
            },
        });
        m_texture = m_asset_service->get_texture("test.png");

        /* poll window size */
        Vec2i display_size = display.get_size(m_window);
        LOG_INFO("display size: {0}", display_size);
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
                LOG_TRACE("resized: {}", ptr->size);

                /* skip invalid window sizes for rendering */
                if (ptr->size.x <= 0 || ptr->size.y <= 0) {
                    continue;
                }
                m_renderer->on_screen_resized(ptr->size);
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

        Vec2 move{};
        m_input_service.pad_foreach([&](Input::Gamepad::DeviceID gamepad) {
            if (m_input_service.is_pad_button_just_pressed(gamepad, Input::Gamepad::Buttons::SOUTH)) {
                LOG_TRACE("Pad {0} just pressed X.", gamepad);
            } else if (m_input_service.is_pad_button_just_released(gamepad, Input::Gamepad::Buttons::SOUTH)) {
                LOG_TRACE("Pad {0} just released X.", gamepad);
            }

            /* move player with gamepad analog stick */
            auto stick_vector = m_input_service.get_pad_stick(gamepad, false);
            move += stick_vector;
        });

        constexpr float PLAYER_SPEED = 60 * 3.0f;
        if (m_input_service.is_key_pressed(Input::Key::Scancode::A)) {
            move.x = -1.0f;
        } else if (m_input_service.is_key_pressed(Input::Key::Scancode::D)) {
            move.x = 1.0f;
        }

        if (m_input_service.is_key_pressed(Input::Key::Scancode::W)) {
            move.y = -1.0f;
        } else if (m_input_service.is_key_pressed(Input::Key::Scancode::S)) {
            move.y = 1.0f;
        }

        if (m_input_service.is_key_pressed(Input::Key::Scancode::Q)) {
            m_camera.rotation -= 0.6f * (float)std::numbers::pi * delta;
        } else if (m_input_service.is_key_pressed(Input::Key::Scancode::E)) {
            m_camera.rotation += 0.6f * (float)std::numbers::pi * delta;
        }

        if (m_input_service.is_key_pressed(Input::Key::Scancode::KP_PLUS)) {
            m_camera.zoom += 1.0f * delta;
        } else if (m_input_service.is_key_pressed(Input::Key::Scancode::KP_MINUS)) {
            m_camera.zoom -= 1.0f * delta;
        }

        m_camera.zoom += 0.1f * m_input_service.get_mouse_wheel_y();
        m_camera.zoom = std::min(m_camera.zoom, 2.0f);
        m_camera.zoom = std::max(m_camera.zoom, 0.1f);

        /* clamp stick and add it into player position */
        move.limit_length(1.0f);

        float cam_cos = std::cosf(m_camera.rotation);
        float cam_sin = std::sinf(m_camera.rotation);
        m_entity.pos.x += ((move.x * cam_cos) - (cam_sin * move.y)) * (PLAYER_SPEED * delta);
        m_entity.pos.y += ((move.x * cam_sin) + (cam_cos * move.y)) * (PLAYER_SPEED * delta);

        m_camera.position.x = std::floor(m_entity.pos.x + 100.0f);
        m_camera.position.y = std::floor(m_entity.pos.y + 100.0f);
    }

    void on_render() override {
        m_queue.clear();

        /* TODO: verify if texture actually exists. */
        m_queue.draw_color(Color{88, 90, 220}, {0, 0}, {400, 400});
        m_queue.draw_color(Color{48, 45, 120, 80}, {40, 40}, {320, 320});

        Vec2i draw_pos{Vec2{std::floor(m_entity.pos.x), std::floor(m_entity.pos.y)}};
        m_queue.draw_texture(m_texture.get(), draw_pos, {200, 200});

        Vec2 mouse_pos = m_input_service.get_mouse_pos();

        constexpr std::array<Viewport, 4> viewports{{
            {.position = {0.0f, 0.0f}, .size = {0.5f, 0.5f}},
            {.position = {0.5f, 0.0f}, .size = {0.5f, 0.5f}},
            {.position = {0.0f, 0.5f}, .size = {0.5f, 0.5f}},
            {.position = {0.5f, 0.5f}, .size = {0.5f, 0.5f}},
        }};

        m_renderer->clear(Color{25, 25, 25, 0});
        for (const auto &viewport : viewports) {
            Viewport abs_viewport = m_renderer->get_absolute_viewport(viewport);

            ViewContext2D ctx = Camera2DUtils::build_view_context(m_camera, abs_viewport);

            auto world_1 = ctx.screen_to_world(mouse_pos);
            auto mouse_alt = ctx.world_to_screen(world_1);
            auto world_2 = ctx.screen_to_world(mouse_alt);

            world_2.x -= 4;
            world_2.y -= 4;
            m_queue.draw_color(Color{0xFF, 0x00, 0x00, 0x60}, Vec2i{world_2}, {16, 16});
            m_queue.draw_color(Color{0xFF, 0x00, 0xFF, 0xFF}, Vec2i{world_1}, {8, 8});

            auto bounds = ctx.get_bounds_aabb();
            m_queue.draw_color(
                Color{0xFF, 0x00, 0xFF, 0x10},
                Vec2i{bounds.tl} + Vec2i{8, 8},
                Vec2i{bounds.size()} - Vec2i{16, 16});

            m_renderer->render_pass(ctx, m_queue);
        }
        m_renderer->swap_buffers();
    }

    void on_close() override {
        m_input_service.close();

        m_texture.reset();
        m_asset_service.reset();

        m_renderer.reset();

        m_platform->display().close_display(m_window);
    }

private:
    struct Entity {
        Vec2 pos;
    };

    Entity m_entity;
    DisplayWindowHandle m_window;
    std::unique_ptr<Renderer> m_renderer;

    TextureRef m_texture;
    RenderQueue m_queue;

    InputService m_input_service;
    std::optional<AssetService> m_asset_service;

    Camera2D m_camera;
};

SET_APP(SandboxApplication)
