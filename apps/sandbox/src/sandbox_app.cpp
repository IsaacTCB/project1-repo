#include <cmath>
#include <vector>

#include "core/camera.h"
#include "core/input/keyboard.h"
#include "core/logger.h"
#include "core/math/random.h"
#include "core/math/vec2.h"
#include "core/math/vec2i.h"
#include "core/view_context_2d.h"

#include "renderer/api/render_queue.h"

#include "runtime/app/application.h"
#include "runtime/app/context/frame_context.h"
#include "runtime/asset_service.h"
#include "runtime/camera_2d_utils.h"
#include "runtime/input_service.h"
#include "runtime/system/particle_system_2d.h"

#include "entity.h"

ParticleEffect2D s_particle{
    .lifetime_min = 0.25f,
    .lifetime_max = 4.0f,

    .emission_rate = 120.0f,

    .speed_min = 50.0f,
    .speed_max = 70.0f,

    .spread = 0.5f,

    .color_start = Color::WHITE,
    .color_end = Color::RED,
};

class SandboxApplication : public IApplication {
    DECLARE_APP(SandboxApplication);

public:
    void on_init() override {
        m_texture = m_servers.asset.get_texture("test.png");

        m_entities.emplace_back(Vec2{0.0f, 0.0f}, 1.0f);

        Random::randomize();

        m_camera.zoom = 0.8f;
        m_camera.anchor = Camera2D::AnchorMode::CENTER;

        auto handle = m_particles.create_effect(s_particle);
        m_emitter = m_particles.create_emitter(handle);
        m_emitter.set_emitting(true);
        m_emitter.set_rotation(2.0f);
    }

    void on_update(UpdateContext &ctx) override {
        float delta = ctx.frame.delta;

        /* this view context object gets recycled
         * by the on_render() function! this way
         * we don't need to waste time building
         * the same view context twice */
        m_view_context = Camera2DUtils::build_view_context(m_camera, ctx.frame.viewport);

        if (m_servers.input.is_key_just_pressed(Input::Key::Scancode::SPACE)) {
            LOG_TRACE("Damn");
        }

        if (m_servers.input.is_key_just_released(Input::Key::Scancode::SPACE)) {
            LOG_TRACE("Undamn");
        }

        Vec2 mouse_pos = get_mouse_coords();
        if (m_servers.input.is_mouse_button_just_pressed(Input::Mouse::Buttons::LEFT)) {
            float speed_scale = Random::randf_range(0.85f, 1.2f);
            m_entities.emplace_back(Vec2(mouse_pos), speed_scale);
        }

        /* update emitter */
        m_emitter.set_position(mouse_pos);
        static float m_angle = 0.0f;
        m_angle += delta * 6.0f;
        m_emitter.set_rotation(m_angle);

        if (m_servers.input.is_mouse_button_just_released(Input::Mouse::Buttons::RIGHT)) {
            LOG_TRACE("Walked");
        }

        if (m_servers.input.is_key_just_pressed(Input::Key::Scancode::T)) {
            m_emitter.reset();
        }

        Vec2 move{};
        m_servers.input.pad_foreach([&](Input::Gamepad::DeviceID gamepad) {
            if (m_servers.input.is_pad_button_just_pressed(gamepad, Input::Gamepad::Buttons::SOUTH)) {
                LOG_TRACE("Pad {0} just pressed X.", gamepad);
            } else if (m_servers.input.is_pad_button_just_released(gamepad, Input::Gamepad::Buttons::SOUTH)) {
                LOG_TRACE("Pad {0} just released X.", gamepad);
            }

            /* move player with gamepad analog stick */
            auto stick_vector = m_servers.input.get_pad_stick(gamepad, false);
            move += stick_vector;
        });

        constexpr float PLAYER_SPEED = 60 * 3.0f;
        if (m_servers.input.is_key_pressed(Input::Key::Scancode::A)) {
            move.x = -1.0f;
        } else if (m_servers.input.is_key_pressed(Input::Key::Scancode::D)) {
            move.x = 1.0f;
        }

        if (m_servers.input.is_key_pressed(Input::Key::Scancode::W)) {
            move.y = -1.0f;
        } else if (m_servers.input.is_key_pressed(Input::Key::Scancode::S)) {
            move.y = 1.0f;
        }

        if (m_servers.input.is_key_pressed(Input::Key::Scancode::Q)) {
            m_camera.rotation -= 0.6f * (float)std::numbers::pi * delta;
        } else if (m_servers.input.is_key_pressed(Input::Key::Scancode::E)) {
            m_camera.rotation += 0.6f * (float)std::numbers::pi * delta;
        }

        if (m_servers.input.is_key_pressed(Input::Key::Scancode::KP_PLUS)) {
            m_camera.zoom += 1.0f * delta;
        } else if (m_servers.input.is_key_pressed(Input::Key::Scancode::KP_MINUS)) {
            m_camera.zoom -= 1.0f * delta;
        }

        m_camera.zoom += 0.1f * m_servers.input.get_mouse_wheel_y();
        m_camera.zoom = std::min(m_camera.zoom, 2.0f);
        m_camera.zoom = std::max(m_camera.zoom, 0.1f);

        /* clamp stick and add it into player position */
        move.limit_length(1.0f);

        float cam_cos = std::cosf(m_camera.rotation);
        float cam_sin = std::sinf(m_camera.rotation);
        for (auto &entity : m_entities) {
            entity.pos.x += ((move.x * cam_cos) - (cam_sin * move.y)) * (PLAYER_SPEED * delta) * entity.speed_scale;
            entity.pos.y += ((move.x * cam_sin) + (cam_cos * move.y)) * (PLAYER_SPEED * delta) * entity.speed_scale;
        }

        m_camera.position.x = std::floor(m_entities[0].pos.x + 100.0f);
        m_camera.position.y = std::floor(m_entities[0].pos.y + 100.0f);

        m_particles.update(delta);
    }

    void on_draw(RenderContext &ctx) override {
        ctx.view_context = &m_view_context;

        /* TODO: verify if texture actually exists. */
        ctx.queue.draw_color(Color{88, 90, 220}, {0, 0}, {400, 400});
        ctx.queue.draw_color(Color{48, 45, 120, 80}, {40, 40}, {320, 320});

        for (const auto &entity : m_entities) {
            Vec2i draw_pos{entity.pos};

            /* get scale size */
            Vec2 scale{200.0f, 200.0f};
            float scale_factor = entity.speed_scale;
            scale_factor *= scale_factor * scale_factor;

            ctx.queue.draw_texture(m_texture.get(), draw_pos, Vec2i(scale * scale_factor));
        }

        Vec2i mouse_pos{m_servers.input.get_mouse_pos()};

        /* draw spot */
        Vec2 world_1 = m_view_context.screen_to_world(Vec2{mouse_pos});
        Vec2 mouse_alt = m_view_context.world_to_screen(world_1);
        Vec2 world_2 = m_view_context.screen_to_world(mouse_alt);

        world_2.x -= 4;
        world_2.y -= 4;
        ctx.queue.draw_color(Color{0xFF, 0x00, 0x00, 0x60}, Vec2i{world_2}, {16, 16});
        ctx.queue.draw_color(Color{0xFF, 0x00, 0xFF, 0xFF}, Vec2i{world_1}, {8, 8});

        auto bounds = m_view_context.get_bounds_aabb();
        ctx.queue.draw_color(
            Color{0xFF, 0x00, 0xFF, 0x20},
            Vec2i{bounds.tl} + Vec2i{8, 8},
            Vec2i{bounds.size()} - Vec2i{16, 16});

        m_particles.render(ctx.queue);
    }

    void on_close() override {
        LOG_INFO("Application ended!");
    }

    [[nodiscard]]
    Vec2 get_mouse_coords() {
        Vec2 screen_pos = m_servers.input.get_mouse_pos();
        return m_view_context.screen_to_world(screen_pos);
    }

private:
    std::vector<Entity> m_entities;
    TextureRef m_texture;
    Camera2D m_camera;

    ViewContext2D m_view_context;

    ParticleSystem2D m_particles;
    ParticleEmitter2D m_emitter;
};

SET_APP(SandboxApplication);
