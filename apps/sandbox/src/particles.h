#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <utility>

#include "core/math/color.h"
#include "core/math/vec2.h"
#include "core/utils/handles.h"
#include "renderer/api/render_queue.h"

#include "particles_emitter.h"

using EffectHandle = Handle;
struct ParticleEffect2D {
    float lifetime_min;
    float lifetime_max;

    /* how many times per second this particle will get generated? */
    float emission_rate;

    float speed_min;
    float speed_max;

    float spread;

    Color color_start = Color::WHITE;
    Color color_end = Color::WHITE;
};

class ParticleSystem2D {
private:
    struct EmitterStorage {
        EmitterHandle effect;
        Vec2 position;
        float rotation;

        float emission_rate;
        float emission_accumulator;

        bool emitting;
    };

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
        float rate = m_effects.has(effect)
                       ? m_effects.get(effect)->emission_rate
                       : 0.0f;

        auto handle = m_emitters.emplace(EmitterStorage{
            .effect = effect,
            .position = {},
            .rotation = 0.0f,
            .emission_rate = rate,
            .emission_accumulator = {},
            .emitting = false,
        });

        return ParticleEmitter2D{
            this,
            handle,
        };
    }

    void destroy_emitter(EmitterHandle handle) {
        m_emitters.remove(handle);
    }

    void emitter_set_rate(EmitterHandle emitter, float emission_rate) {
        EmitterStorage *instance = m_emitters.get(emitter);
        if (!instance) {
            return;
        }

        instance->emission_rate = emission_rate;
    }

    void emitter_set_position(EmitterHandle emitter, const Vec2 &position) {
        EmitterStorage *instance = m_emitters.get(emitter);
        if (!instance) {
            return;
        }

        instance->position = position;
    }

    void emitter_set_angle(EmitterHandle emitter, float angle) {
        EmitterStorage *instance = m_emitters.get(emitter);
        if (!instance) {
            return;
        }

        instance->rotation = angle;
    }

    void emitter_set_emitting(EmitterHandle emitter, bool emitting) {
        EmitterStorage *instance = m_emitters.get(emitter);
        if (!instance) {
            return;
        }

        instance->emitting = emitting;
    }

private:
    void update_emitters(float delta);
    void update_particles(float delta);
    void emit(EmitterStorage &emitter);

private:
    static constexpr size_t PARTICLE_COUNT = 1000;

    struct ParticleStorage {
        std::array<Vec2, PARTICLE_COUNT> pos;
        std::array<Vec2, PARTICLE_COUNT> vel;
        std::array<float, PARTICLE_COUNT> lifetime;
        std::array<float, PARTICLE_COUNT> lifetime_remaining;
        std::array<bool, PARTICLE_COUNT> alive{};
    };

    std::unique_ptr<ParticleStorage> m_particles;
    HandlePool<EmitterStorage> m_emitters;
    HandlePool<ParticleEffect2D> m_effects;

    size_t m_index{};
};
