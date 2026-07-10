#include <vector>

#include "core/logger.h"
#include "core/math/random.h"
#include "core/math/vec2.h"
#include "core/math/vec2i.h"

#include "renderer/api/render_queue.h"
#include "runtime/app/application.h"
#include "runtime/asset_service.h"
#include "runtime/input_service.h"

#include "entity.h"

class SandboxApplication : public IApplication {
    DECLARE_APP(SandboxApplication);

public:
    void on_init() override {
        m_texture = m_servers.asset.get_texture("test.png");

        m_entities.emplace_back(Vec2{0.0f, 0.0f}, 1.0f);

        Random::randomize();
    }

    void on_update(float delta) override {
        if (m_servers.input.is_key_just_pressed(Input::Key::Scancode::SPACE)) {
            LOG_TRACE("Damn");
        }

        if (m_servers.input.is_key_just_released(Input::Key::Scancode::SPACE)) {
            LOG_TRACE("Undamn");
        }

        Vec2i mouse_pos{m_servers.input.get_mouse_pos()};
        if (m_servers.input.is_mouse_button_just_pressed(Input::Mouse::Buttons::LEFT)) {
            float speed_scale = Random::randf_range(0.85f, 1.2f);
            m_entities.emplace_back(Vec2(mouse_pos), speed_scale);
        }
        if (m_servers.input.is_mouse_button_just_released(Input::Mouse::Buttons::RIGHT)) {
            LOG_TRACE("Walked");
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

        /* clamp stick and add it into player position */
        move.limit_length(1.0f);

        for (auto &entity : m_entities) {
            entity.pos += move * PLAYER_SPEED * delta * entity.speed_scale;
        }
    }

    void on_draw(RenderQueue &queue) override {
        /* TODO: verify if texture actually exists. */
        queue.draw_color(Color{88, 90, 220}, {0, 0}, {400, 400});
        queue.draw_color(Color{48, 45, 120, 80}, {40, 40}, {320, 320});

        for (const auto &entity : m_entities) {
            Vec2i draw_pos{entity.pos};

            /* get scale size */
            Vec2 scale{200.0f, 200.0f};
            float scale_factor = entity.speed_scale;
            scale_factor *= scale_factor * scale_factor;

            queue.draw_texture(m_texture.get(), draw_pos, Vec2i(scale * scale_factor));
        }

        Vec2i mouse_pos{m_servers.input.get_mouse_pos()};
        queue.draw_color(Color{0xFF, 0x0, 0xFF}, mouse_pos, {8, 8});
    }

    void on_close() override {
        LOG_INFO("Application ended!");
    }

private:
    std::vector<Entity> m_entities;
    TextureRef m_texture;
};

SET_APP(SandboxApplication);
