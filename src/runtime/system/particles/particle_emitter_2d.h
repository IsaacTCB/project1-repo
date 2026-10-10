#pragma once

#include <cstdint>

#include "core/math/vec2.h"
#include "core/utils/handle_pool.h"
#include "core/utils/handles.h"

#include "particle_handles.h"

class Vec2;
class EmitterPool;

/* ===================== */
/*    ParticleEmitter2D  */
/* ===================== */

class ParticleEmitter2D {
public:
    ParticleEmitter2D() = default;
    ~ParticleEmitter2D();
    ParticleEmitter2D(const ParticleEmitter2D &) = delete;
    ParticleEmitter2D &operator=(const ParticleEmitter2D &) = delete;
    ParticleEmitter2D(ParticleEmitter2D &&other) noexcept;
    ParticleEmitter2D &operator=(ParticleEmitter2D &&other) noexcept;

    void set_position(const Vec2 &position);
    void set_rotation(float angle_radius);
    void set_emitting(bool emitting);

    void reset() {
        destroy_handle();
        m_emitter_pool = nullptr;
        m_handle = {};
    }

protected:
    friend EmitterPool;
    ParticleEmitter2D(EmitterPool *system, EmitterHandle handle)
        : m_emitter_pool(system), m_handle(handle) {}

private:
    void destroy_handle();

private:
    EmitterPool *m_emitter_pool{nullptr};
    EmitterHandle m_handle{};
};

/* ================= */
/*    EmitterPool    */
/* ================= */

class EmitterPool {
public:
    struct EmitterData {
        EffectHandle effect; /* effect particle handle */
        Vec2 position;
        float rotation;
        uint32_t emission_rate;

        /* emitter state */
        float emission_accumulator;
        bool emitting;
    };

public:
    template <typename T>
    ParticleEmitter2D create(T &&data) {
        return {
            this,
            m_pool.emplace(std::forward<T>(data)),
        };
    }

    void destroy(EmitterHandle emitter) {
        m_pool.remove(emitter);
    }

    using EmitCallbackFn = void (*)(EmitterData &emitted, void *user);
    void update(float delta, EmitCallbackFn emit_callback, void *user) {
        m_pool.for_each([emit_callback, user, delta](EmitterData &emitter) -> void {
            if (!emitter.emitting) {
                return;
            }

            /* update emitter */
            emitter.emission_accumulator += delta * static_cast<float>(emitter.emission_rate);
            while (emitter.emission_accumulator >= 1.0f) {
                emit_callback(emitter, user);
                emitter.emission_accumulator -= 1.0f;
            }
        });
    }

protected:
    void set_rate(EmitterHandle emitter, uint32_t emission_rate);
    void set_position(EmitterHandle emitter, const Vec2 &position);
    void set_angle(EmitterHandle emitter, float angle);
    void set_emitting(EmitterHandle emitter, bool emitting);

    friend ParticleEmitter2D;

private:
    HandlePool<EmitterData, EmitterHandle> m_pool;
};
