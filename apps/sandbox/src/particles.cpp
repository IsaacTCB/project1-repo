#include "particles.h"

#include "core/math/random.h"
#include "core/math/vec2.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>

ParticleSystem2D::ParticleSystem2D() {
    m_particles = std::make_unique<ParticleStorage>();
}

void ParticleSystem2D::update(float delta) {
    update_emitters(delta);
    update_particles(delta);
}

void ParticleSystem2D::render(RenderQueue &queue) {
    for (size_t i = 0; i < PARTICLE_COUNT; ++i) {
        if (!m_particles->alive[i]) {
            continue;
        }

        float percent = m_particles->lifetime_remaining[i] /
                        m_particles->lifetime[i];

        queue.draw_color(Color{255, 255, 255, (uint8_t)((float)120 * percent)}, Vec2i{m_particles->pos[i]}, Vec2i{Vec2{16.0f, 16.0f} * percent});
    }
}

/* private */

void ParticleSystem2D::update_emitters(float delta) {
    m_emitters.foreach ([this, delta](EmitterStorage &emitter) {
        if (!emitter.emitting) {
            return;
        }

        /* update emitter */
        emitter.emission_accumulator += delta * emitter.emission_rate;
        while (emitter.emission_accumulator >= 1.0f) {
            emit(emitter);
            emitter.emission_accumulator -= 1.0f;
        }
    });
}

void ParticleSystem2D::update_particles(float delta) {
    for (size_t i = 0; i < PARTICLE_COUNT; ++i) {
        if (!m_particles->alive[i]) {
            continue;
        }

        /* update position */
        Vec2 &pos = m_particles->pos[i];
        pos += m_particles->vel[i] * delta;

        /* update lifetime */
        m_particles->lifetime_remaining[i] -= delta;
        if (m_particles->lifetime_remaining[i] <= 0.0f) {
            m_particles->alive[i] = false;
        }
    }
}

void ParticleSystem2D::emit(ParticleSystem2D::EmitterStorage &emitter) {
    const ParticleEffect2D *effect = m_effects.get(emitter.effect);
    if (!effect) {
        return;
    }

    /* set lifetime */
    const float total_lifetime = Random::randf_range(
        effect->lifetime_min, effect->lifetime_max);
    m_particles->lifetime_remaining[m_index] =
        m_particles->lifetime[m_index] = total_lifetime;

    float angle = emitter.rotation +
                  Random::randf_range(-effect->spread, effect->spread);

    float initial_speed = Random::randf_range(effect->speed_min, effect->speed_max);
    Vec2 initial_velocity = Vec2{
        std::cos(angle) * initial_speed,
        std::sin(angle) * initial_speed,
    };
    m_particles->vel[m_index] = initial_velocity;

    /* apply emitter state */
    m_particles->pos[m_index] = emitter.position;
    m_particles->alive[m_index] = true;

    m_index = (m_index + 1) % PARTICLE_COUNT;
}

/* emitter */

ParticleEmitter2D::~ParticleEmitter2D() {
    destroy_handle();
}

ParticleEmitter2D::ParticleEmitter2D(ParticleEmitter2D &&other) noexcept
    : m_system(other.m_system), m_handle(other.m_handle) {
    other.m_system = nullptr;
    other.m_handle = {};
}

ParticleEmitter2D &ParticleEmitter2D::operator=(ParticleEmitter2D &&other) noexcept {
    if (this == &other) {
        return *this;
    }

    destroy_handle();
    m_system = other.m_system;
    m_handle = other.m_handle;
    other.m_system = nullptr;
    other.m_handle = {};
    return *this;
}

void ParticleEmitter2D::set_position(const Vec2 &position) {
    m_system->emitter_set_position(m_handle, position);
}

void ParticleEmitter2D::set_rotation(float angle_radius) {
    m_system->emitter_set_angle(m_handle, angle_radius);
}

void ParticleEmitter2D::set_emitting(bool emitting) {
    m_system->emitter_set_emitting(m_handle, emitting);
}

void ParticleEmitter2D::destroy_handle() {
    if (!m_system || !m_handle) {
        return;
    }
    m_system->destroy_emitter(m_handle);
}
