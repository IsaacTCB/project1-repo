#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

#include "core/math/color.h"
#include "core/math/vec2.h"
#include "core/utils/handles.h"
#include "renderer/api/render_queue.h"

#include "../asset_service.h"
#include "particle_emitter_2d.h"

using EffectHandle = Handle;
struct ParticleEffect2D {
    float lifetime_min;
    float lifetime_max;

    /* how many times per second this particle will get generated? */
    uint32_t emission_rate;
    float emission_spread = 0.0f;

    float speed_min = 1.0f;
    float speed_max = 1.0f;

    /* color transition state */
    Color color_start = Color::WHITE;
    Color color_end = Color::WHITE;

    /* size scale */
    float scale_start = 1.0f;
    float scale_end = 0.2f;

    /* particle props */
    TextureRef particle_texture;
    Vec2 particle_size{16.0f, 16.0f};
    Vec2 particle_gravity{0.0f, 0.0f};
};

class ParticleSystem2D {
public:
    ParticleSystem2D();

    void update(float delta);
    void render(RenderQueue &queue);

public:
    template <typename Fx>
    EffectHandle create_effect(Fx &&effect) {
        return m_effects.emplace(std::forward<Fx>(effect));
    }

    void destroy_effect(EffectHandle handle) {
        m_effects.remove(handle);
    }

    [[nodiscard]]
    ParticleEmitter2D create_emitter(EffectHandle effect) {
        /* HACK: don't do this */
        uint32_t rate = m_effects.has(effect)
                          ? m_effects.get(effect)->emission_rate
                          : 0;

        return m_emitters.create(EmitterPool::EmitterData{
            .effect = effect,
            .position = {},
            .rotation = 0.0f,
            .emission_rate = rate,
            .emission_accumulator = {},
            .emitting = false,
        });
    }

private:
    void update_emitters(float delta);
    void update_particles(float delta);
    void emit(EmitterPool::EmitterData &emitter);

    template <typename Func>
    void particle_foreach(Func &&func) {
        for (size_t p{}; p < PARTICLE_COUNT; ++p) {
            if (!m_particles->alive[p]) {
                continue;
            }
            std::invoke(func, p);
        }
    }

private:
    static constexpr size_t PARTICLE_COUNT = 2000;
    struct ParticleStorage {
        std::array<Vec2, PARTICLE_COUNT> pos;
        std::array<Vec2, PARTICLE_COUNT> vel;
        std::array<float, PARTICLE_COUNT> lifetime;
        std::array<float, PARTICLE_COUNT> lifetime_remaining;
        std::array<EffectHandle, PARTICLE_COUNT> effect{};
        std::array<bool, PARTICLE_COUNT> alive{};
    };

    std::unique_ptr<ParticleStorage> m_particles;
    size_t m_index{};

    EmitterPool m_emitters;
    HandlePool<ParticleEffect2D> m_effects;
};
