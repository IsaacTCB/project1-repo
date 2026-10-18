#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>

#include <core/camera.h>
#include <core/input/gamepad.h>
#include <core/input/keyboard.h>
#include <core/logger.h>
#include <core/math/color.h>
#include <core/math/random.h>
#include <core/math/vec2.h>
#include <core/math/vec2i.h>
#include <core/view_context_2d.h>

#include <runtime/app/application.h>
#include <runtime/app/context/frame_context.h>
#include <runtime/asset_service.h>
#include <runtime/camera_2d_utils.h>
#include <runtime/input_service.h>

#include <entt/entity/registry.hpp>

#include "components.h"
#include "runtime/system/particles/particle_emitter_2d.h"
#include "runtime/system/particles/particle_system_2d.h"
#include "utils/gridmap.h"
#include "utils/input.h"

using Tile = uint16_t;
using Tilemap = GridMap<Tile>;

constexpr size_t TILE_SIZE = 32;

constexpr auto m_tilecolor = ([]() constexpr {
    std::array<Color, 4> arr;
    arr[0] = Color{255, 0, 255}; /* air, typically unused */
    arr[1] = Color{40, 40, 40};
    arr[2] = Color{60, 60, 60};
    arr[3] = Color{80, 80, 80};
    return arr;
})();

std::optional<Tilemap> m_tilemap;

Tile get_tilemap_at_pos(Vec2 pos) {
    auto tilepos = Vec2i{
        (int)std::floor(pos.x / (float)TILE_SIZE),
        (int)std::floor(pos.y / (float)TILE_SIZE),
    };

    size_t twidth = m_tilemap->width();
    size_t theight = m_tilemap->height();

    if ((tilepos.x < 0 || (size_t)tilepos.x >= twidth) ||
        (tilepos.y < 0 || (size_t)tilepos.y >= theight)) {
        return Tile{};
    }

    size_t idx = (twidth * tilepos.y) + tilepos.x;
    return *(m_tilemap->data() + idx);
}

enum class InputAction : std::uint8_t {
    /* player movement */
    LEFT,
    RIGHT,
    UP,
    DOWN,

    /* player actions */
    JUMP,

    /* COUNT is required */
    COUNT,
};

class Platformer2D : public IApplication {
    DECLARE_APP(Platformer2D);

public:
    void configure(ApplicationConfig &config) override {
        config.display = {
            .caption = "Platformer 2D",
            .size = {1200, 600},
            .resizable = true,
        };
    }

    void on_init() override {
        m_camera.zoom = 1.0f;
        m_camera.anchor = Camera2D::AnchorMode::CENTER;

        m_input_action.bind(
            InputAction::LEFT,
            InputActionBind{
                .keys = {Input::Key::Scancode::LEFT, Input::Key::Scancode::A},
                .buttons = Input::Gamepad::Buttons::DPAD_LEFT,
            });
        m_input_action.bind(
            InputAction::RIGHT,
            InputActionBind{
                .keys = {Input::Key::Scancode::RIGHT, Input::Key::Scancode::D},
                .buttons = Input::Gamepad::Buttons::DPAD_RIGHT,
            });
        m_input_action.bind(
            InputAction::UP,
            InputActionBind{
                .keys = {Input::Key::Scancode::UP, Input::Key::Scancode::W},
                .buttons = Input::Gamepad::Buttons::DPAD_UP,
            });
        m_input_action.bind(
            InputAction::DOWN,
            InputActionBind{
                .keys = {Input::Key::Scancode::DOWN, Input::Key::Scancode::S},
                .buttons = Input::Gamepad::Buttons::DPAD_DOWN,
            });
        m_input_action.bind(
            InputAction::JUMP,
            InputActionBind{
                .keys = {Input::Key::Scancode::SPACE, Input::Key::Scancode::UNKNOWN},
                .buttons = Input::Gamepad::Buttons::SOUTH,
            });

        m_vignette_texture = m_servers.asset.get_texture("vignette.png");

        Random::randomize();

        m_tilemap.emplace(TILE_SIZE, TILE_SIZE);
        m_tilemap->fill_rect(0, 0, m_tilemap->width(), m_tilemap->height(), 0);

        m_tilemap->fill_rect(4, 8, 16, 32, 1);
        m_tilemap->fill_rect(5, 9, 14, 32, 2);
        m_tilemap->fill_rect(6, 10, 12, 32, 3);

        m_player_entity = m_registry.create();
        auto &transform = m_registry.emplace<TransformComponent>(m_player_entity);
        transform.position = Vec2{30.0f, 30.0f};

        m_registry.emplace<VelocityComponent>(m_player_entity);
        m_registry.emplace<InputComponent>(m_player_entity);

        auto &rect = m_registry.emplace<CanvasRectComponent>(m_player_entity);
        rect.size = Vec2i{32, 32};
        rect.color = Color{70, 50, 160};

        m_particles.seed();
        m_particle_fx = m_particles.create_effect(ParticleEffect2D{
            .lifetime_min = 0.2f,
            .lifetime_max = 0.5f,

            /* how many times per second this particle will get generated? */
            .emission_rate = 40,
            .emission_spread = 0.2f,

            .speed_min = 60.0f,
            .speed_max = 100.0f,

            /* color transition state */
            .color_start = Color{0xFFFFFFFF},
            .color_end = Color{0xFFFFFF00},

            /* size scale */
            .scale_start = 0.7f,
            .scale_end = 0.2f,

            /* particle props */
            .particle_texture{},
            .particle_size{16.0f, 16.0f},
            .particle_gravity{0.0f, -2.0f},
        });

        m_emitter = m_particles.create_emitter(m_particle_fx);
        m_emitter.set_emitting(false);
    }

    void on_update(UpdateContext &ctx) override {
        float delta = ctx.frame.delta;

        /* this view context object gets recycled
         * by the on_render() function! this way
         * we don't need to waste time building
         * the same view context twice */
        m_view_context = Camera2DUtils::build_view_context(m_camera, *ctx.frame.view_layout);

        /* update the input action system */
        m_input_action.update(m_servers.input);

        m_camera.zoom = 1.0f;
        m_registry.view<VelocityComponent, InputComponent>().each([&]([[maybe_unused]] auto entity, auto &vel) {
            Vec2 move{
                m_input_action.get_axis(InputAction::LEFT, InputAction::RIGHT),
                m_input_action.get_axis(InputAction::UP, InputAction::DOWN),
            };
            m_servers.input.pad_foreach([&](Input::Gamepad::DeviceID device) {
                move += m_servers.input.get_pad_stick(device, false);

                Vec2 look = m_servers.input.get_pad_stick(device, true);
                m_camera.rotation += ((look.x * 0.5f) - m_camera.rotation) * delta * 1.0f;
            });
            move.limit_length(1.0f);

            if (move.length() > 0.1f) {
                Vec2 dir = move.normalized();

                float acc = 20.0f;
                if (vel.velocity.length() > 0.1f) {
                    float dot = vel.velocity.normalized().dot(dir);
                    if (dot < -0.8f) {
                        acc = 40.0f;

                        auto &transform = m_registry.get<TransformComponent>(entity);
                        m_emitter.set_position(transform.position + Vec2{0.0f, 12.0f});
                        m_emitter.set_emitting(true);

                        float angl = 3.1415f - std::atan2(vel.velocity.y, vel.velocity.x);
                        m_emitter.set_rotation(angl);
                    } else {
                        m_emitter.set_emitting(false);
                    }
                }

                vel.velocity += dir * acc * delta;
                vel.velocity.limit_length(move.length() * 10.0f);
            } else {
                vel.velocity -= vel.velocity * delta * 10.0f;
            }
        });

        m_registry.view<TransformComponent, VelocityComponent>().each([](auto &transform, auto &vel) {
            // move x
            if (get_tilemap_at_pos(transform.position + Vec2{vel.velocity.x, 0.0f})) {
                vel.velocity.x = 0.0f;
            }
            transform.position.x += vel.velocity.x;

            // move y
            if (get_tilemap_at_pos(transform.position + Vec2{0, vel.velocity.y})) {
                /* TODO: snap */
                vel.velocity.y = 0.0f;
            }
            transform.position.y += vel.velocity.y;

            /* TODO: perform collision checks */
        });

        if (m_registry.all_of<TransformComponent>(m_player_entity)) {
            auto &transform = m_registry.get<TransformComponent>(m_player_entity);
            m_camera.position += (transform.position - m_camera.position) * delta * 2.0f;
        }

        m_particles.update(delta);
    }

    void on_draw(RenderContext &ctx) override {
        ctx.view_context = &m_view_context;
        ctx.clear_color = Color{10, 10, 10, 0};

        draw_checker_background(ctx);

        /* draw map */
        size_t tilemap_width = m_tilemap->width();
        size_t tilemap_height = m_tilemap->height();
        for (size_t y = 0; y < tilemap_height; ++y) {
            for (size_t x = 0; x < tilemap_width; ++x) {
                Tile *t = m_tilemap->data() + (y * tilemap_height) + x;
                if (*t == 0) {
                    continue;
                }
                size_t idx = *t;
                ctx.queue.draw_color(
                    m_tilecolor[idx],
                    Vec2i{(int)TILE_SIZE * (int)x, (int)TILE_SIZE * (int)y},
                    Vec2i{32, 32});
            }
        }

        /* draw characters */
        auto view = m_registry.view<TransformComponent, CanvasRectComponent>();
        for (auto entity : view) {
            const auto &[transform, rect] = view.get<const TransformComponent, const CanvasRectComponent>(entity);

            Vec2i draw_pos{transform.position};
            draw_pos -= rect.size / 2;

            ctx.queue.draw_color(
                rect.color,
                draw_pos,
                rect.size);
        }

        /* draw vignette */
        auto bounds = ctx.view_context->get_bounds_aabb();
        ctx.queue.draw_texture_blended(
            m_vignette_texture.get(),
            Color{0xFFFFFFE0},
            Vec2i{bounds.tl} - Vec2i{1, 1},
            Vec2i{(int)bounds.width() + 4, (int)bounds.height() + 4});

        /* draw cursor pointer */
        Vec2 mouse_pos = get_mouse_coords();
        ctx.queue.draw_color(Color{255, 0, 0, 255}, Vec2i{mouse_pos}, Vec2i{8, 8});

        m_particles.render(ctx.queue);
    }

    void draw_checker_background(RenderContext &ctx) const {
        /* FIXME: not an app problem, but an engine one:
         * currently our OpenGL renderer does not support any
         * batching, so each rectangle will consume ONE draw
         * call, which is wasteful since we are rendering
         * hundreds of rectangles. */
        constexpr float PARALLAX = 0.8f;
        const Vec2 parallax_offset = m_camera.position * PARALLAX;

        const auto bounds = ctx.view_context->get_bounds_aabb();
        const auto tx = static_cast<int>(std::floor((bounds.tl.x - parallax_offset.x) / 16.0f));
        const auto ty = static_cast<int>(std::floor((bounds.tl.y - parallax_offset.y) / 16.0f));
        const auto tw = static_cast<int>(std::ceil(bounds.width() / 16.0f) + 1);
        const auto th = static_cast<int>(std::ceil(bounds.height() / 16.0f) + 1);
        for (int y = 0; y < th; ++y) {
            for (int x = 0; x < tw; ++x) {
                const int bx = x + tx;
                const int by = y + ty;

                const bool odd = (std::abs(by + bx) % 2) == 1;
                ctx.queue.draw_color(
                    odd ? Color{114, 168, 211}
                        : Color{115, 174, 210},
                    Vec2i{
                        (bx * 16) + static_cast<int>(parallax_offset.x),
                        (by * 16) + static_cast<int>(parallax_offset.y),
                    },
                    Vec2i{16, 16});
            }
        }
    }

    void on_close() override {
        m_particles.destroy_effect(m_particle_fx);
        m_emitter.reset();
    }

    [[nodiscard]]
    Vec2 get_mouse_coords() {
        Vec2 screen_pos = m_servers.input.get_mouse_pos();
        return m_view_context.screen_to_world(screen_pos);
    }

private:
    Camera2D m_camera;
    ViewContext2D m_view_context;

    TextureRef m_vignette_texture;

    entt::registry m_registry;
    entt::entity m_player_entity;

    InputActionSystem<InputAction> m_input_action;

    ParticleEffectHandle m_particle_fx;
    ParticleEmitter2D m_emitter;
    ParticleSystem2D m_particles;
};

SET_APP(Platformer2D);
