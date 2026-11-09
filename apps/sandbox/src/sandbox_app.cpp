#include <cmath>
#include <numbers>
#include <vector>

#include "core/camera.h"
#include "core/input/keyboard.h"
#include "core/logger.h"
#include "core/math/random.h"
#include "core/math/vec2.h"
#include "core/math/vec2i.h"
#include "core/view_context_2d.h"

#include "runtime/app/application.h"
#include "runtime/app/context/frame_context.h"
#include "runtime/asset_service.h"
#include "runtime/audio/audio_controller.h"
#include "runtime/audio/audio_system.h"
#include "runtime/camera_2d_utils.h"
#include "runtime/input_service.h"

#include "entity.h"

#if ENABLE_IMGUI
#include <imgui.h>
#endif

#define AUDIO_FILEPATH "test.ogg"

constexpr auto AUDIO_LOOP_POINTS = LoopPointsFrames{
    .start = 460239,
    .end = 0,
};

class SandboxApplication : public IApplication {
    DECLARE_APP(SandboxApplication);

public:
    void configure(ApplicationConfig &config) override {
        constexpr Vec2i VIEWPORT_SIZE = {480, 270};

        config.display = {
            .caption = "Sandbox",
            .size = VIEWPORT_SIZE * 3,
            .resizable = true,
        };

        config.view = {
            /* desired camera view size */
            .logical_size = Vec2{VIEWPORT_SIZE},

            /* pixelated viewport, results in a retro feel. */
            .mode = PresentationMode::FIXED_RESOLUTION,

            /* viewport expands to fill the window's total width */
            .aspect = AspectPolicy::KEEP_HEIGHT,
        };
    }

    AudioSourceRef load_sound(const std::string &filepath) {
        auto handle = m_servers.asset.get_audio_source(filepath);
        if (!handle.get()) {
            LOG_WARN("Failed to load audio source {}", filepath);
            return AudioSourceRef{};
        }
        LOG_INFO("Loaded load audio source {}", filepath);
        return handle;
    }

    void on_init() override {
        m_texture = m_servers.asset.get_texture("test.png");
        if (m_texture) {
            LOG_TRACE("loaded texture size is {}x{}", m_texture.width(), m_texture.height());
        } else {
            LOG_WARN("failed to load sandbox texture");
        }

        m_entities.emplace_back(Vec2{0.0f, 0.0f}, 1.0f);

        Random::randomize();

        m_camera.zoom = 0.8f;
        m_camera.anchor = Camera2D::AnchorMode::CENTER;

        m_audio_source = load_sound(AUDIO_FILEPATH);
        if (m_audio_source.get()) {
            auto ch = m_servers.audio->get_source_length_seconds(m_audio_source.get());
            LOG_TRACE("song is {:.1f}s long", ch);

            m_bgm_voice = m_servers.audio->play_audio(m_audio_source.get(),
                {
                    .volume = 0.2f,
                    .pitch = 1.0f,
                    .looping = true,
                    .loop_points = AUDIO_LOOP_POINTS,
                });

            if (m_servers.audio->is_audio_playing(m_bgm_voice)) {
                LOG_TRACE("playing voice!");
            }
        }

        m_click_sound = load_sound("click.wav");
    }

    void on_update(UpdateContext &ctx) override {
        float delta = ctx.frame.delta;

        /* this view context object gets recycled
         * by the on_render() function! this way
         * we don't need to waste time building
         * the same view context twice */
        m_view_context = Camera2DUtils::build_view_context(
            m_camera, *ctx.frame.view_layout);

        if (m_servers.input.is_key_just_pressed(Input::Key::Scancode::SPACE)) {
            LOG_TRACE("Damn");
        }

        if (m_servers.input.is_key_just_released(Input::Key::Scancode::SPACE)) {
            if (m_servers.audio->is_audio_playing(m_bgm_voice)) {
                const bool looping = m_servers.audio->is_audio_looping(m_bgm_voice);
                LOG_INFO("Toggling looping {}...", looping ? "off" : "on");
                m_servers.audio->set_audio_looping(m_bgm_voice, !looping);
            } else {
                m_bgm_voice = m_servers.audio->play_audio(m_audio_source.get(),
                    {
                        .volume = 0.2f,
                        .pitch = 1.0f,
                        .looping = true,
                        .loop_points = AUDIO_LOOP_POINTS,
                    });

                LOG_TRACE("playing bgm...");
            }
        }

        Vec2 mouse_pos = get_mouse_coords();
        if (m_servers.input.is_mouse_button_just_pressed(Input::Mouse::Buttons::LEFT)) {
            float speed_scale = Random::randf_range(0.85f, 1.2f);
            m_entities.emplace_back(Vec2(mouse_pos), speed_scale);

            /* play click sound effect */
            if (m_click_sound.get()) {
                m_servers.audio->play_audio(m_click_sound.get());
            }
        }
        if (m_servers.input.is_mouse_button_just_released(Input::Mouse::Buttons::RIGHT)) {
            LOG_TRACE("Walked");

            if (m_servers.audio->is_audio_playing(m_bgm_voice)) {
                float new_volume = Random::randf_range(0.5f, 1.0f);
                LOG_TRACE("Setting music volume to {}%...", static_cast<int>(new_volume * 100.0f));
                m_servers.audio->set_audio_volume(m_bgm_voice, new_volume);
            }
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

        float cam_cos = std::cos(m_camera.rotation);
        float cam_sin = std::sin(m_camera.rotation);
        for (auto &entity : m_entities) {
            entity.pos.x += ((move.x * cam_cos) - (cam_sin * move.y)) * (PLAYER_SPEED * delta) * entity.speed_scale;
            entity.pos.y += ((move.x * cam_sin) + (cam_cos * move.y)) * (PLAYER_SPEED * delta) * entity.speed_scale;
        }

        m_camera.position.x = std::floor(m_entities[0].pos.x);
        m_camera.position.y = std::floor(m_entities[0].pos.y);

#if ENABLE_IMGUI
        imgui_logic();
#endif
    }

#if ENABLE_IMGUI
    void imgui_logic() {
        ImGui::Begin("Audio Inspector");

        std::string path{AUDIO_FILEPATH};
        ImGui::InputText("Audio File", path.data(), path.size() + 1, ImGuiInputTextFlags_ReadOnly);

        if (m_audio_source.get()) {
            const auto info = m_servers.audio->get_source_info(m_audio_source.get());
            ImGui::Text("channels = %zu", info->channels);
            ImGui::Text("sample rate = %zu", info->sample_rate);
            ImGui::Text("frame count = %zu", info->frame_count);
            ImGui::Text("duration (seconds) = %.2fs", info->duration_seconds());
        }

        bool is_playing = m_bgm_voice && m_servers.audio->is_audio_playing(m_bgm_voice);
        static float s_volume = 0.5f;

        if (ImGui::Checkbox("Playing", &is_playing)) {
            if (is_playing) {
                m_bgm_voice = m_servers.audio->play_audio(m_audio_source.get(),
                    {
                        .volume = s_volume,
                        .pitch = 1.0f,
                        .looping = true,
                        .loop_points = AUDIO_LOOP_POINTS,
                    });
            } else {
                m_servers.audio->stop_audio(m_bgm_voice);
                m_bgm_voice = {};
            }
        }

        ImGui::SeparatorText("Audio Voice");

        if (is_playing) {
            bool looping = m_servers.audio->is_audio_looping(m_bgm_voice);
            if (ImGui::Checkbox("Looping", &looping)) {
                m_servers.audio->set_audio_looping(m_bgm_voice, looping);
            }

            auto loop_points = AUDIO_LOOP_POINTS;
            ImGui::Text("loop start = %zu", loop_points.start);
            ImGui::Text("loop end = %zu", loop_points.end);
            if (ImGui::DragFloat("Volume", &s_volume, 0.01f, 0.0f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
                m_servers.audio->set_audio_volume(m_bgm_voice, s_volume);
            }
        } else {
            ImGui::TextWrapped("Audio voice not currently active!");
        }

        ImGui::End();
    }
#endif

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

            ctx.queue.draw_texture_ex(
                m_texture.handle(),
                Color::WHITE,
                draw_pos,
                Vec2i(scale * scale_factor),
                {0.5f, 0.5f},
                0.0f);
        }

        Vec2i mouse_pos{m_servers.input.get_mouse_pos()};

        /* draw spot */
        Vec2 world_1 = m_view_context.screen_to_world(Vec2{mouse_pos});
        Vec2 mouse_alt = m_view_context.world_to_screen(world_1);
        Vec2 world_2 = m_view_context.screen_to_world(mouse_alt);

        ctx.queue.draw_color_ex(Color{0xFF, 0x00, 0x00, 0x60}, Vec2i{world_2}, {16, 16}, {0.5f, 0.5f}, 0.0f);
        ctx.queue.draw_color_ex(Color{0xFF, 0x00, 0xFF, 0xFF}, Vec2i{world_1}, {8, 8}, {0.5f, 0.5f}, 0.0f);

        auto bounds = m_view_context.get_bounds_aabb();
        ctx.queue.draw_color(
            Color{0xFF, 0x00, 0xFF, 0x20},
            Vec2i{bounds.tl} + Vec2i{8, 8},
            Vec2i{bounds.size()} - Vec2i{16, 16});
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

    AudioSourceRef m_audio_source;
    AudioSourceRef m_click_sound;

    VoiceHandle m_bgm_voice;

    ViewContext2D m_view_context;
};

SET_APP(SandboxApplication);
