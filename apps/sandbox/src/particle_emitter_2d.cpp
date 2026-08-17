#include "particle_emitter_2d.h"

#include "core/math/vec2.h"

/* ===================== */
/*    ParticleEmitter2D  */
/* ===================== */

ParticleEmitter2D::~ParticleEmitter2D() {
    destroy_handle();
}

ParticleEmitter2D::ParticleEmitter2D(ParticleEmitter2D &&other) noexcept
    : m_emitter_pool(other.m_emitter_pool), m_handle(other.m_handle) {
    other.m_emitter_pool = nullptr;
    other.m_handle = {};
}

ParticleEmitter2D &ParticleEmitter2D::operator=(ParticleEmitter2D &&other) noexcept {
    if (this == &other) {
        return *this;
    }

    destroy_handle();
    m_emitter_pool = other.m_emitter_pool;
    m_handle = other.m_handle;
    other.m_emitter_pool = nullptr;
    other.m_handle = {};
    return *this;
}

void ParticleEmitter2D::set_position(const Vec2 &position) {
    m_emitter_pool->set_position(m_handle, position);
}

void ParticleEmitter2D::set_rotation(float angle_radius) {
    m_emitter_pool->set_angle(m_handle, angle_radius);
}

void ParticleEmitter2D::set_emitting(bool emitting) {
    m_emitter_pool->set_emitting(m_handle, emitting);
}

void ParticleEmitter2D::destroy_handle() {
    if (!m_emitter_pool || !m_handle) {
        return;
    }
    m_emitter_pool->destroy(m_handle);
}

/* ================= */
/*    EmitterPool    */
/* ================= */

void EmitterPool::set_rate(EmitterHandle emitter, float emission_rate) {
    EmitterData *instance = m_pool.get(emitter);
    if (!instance) {
        return;
    }

    instance->emission_rate = emission_rate;
}

void EmitterPool::set_position(EmitterHandle emitter, const Vec2 &position) {
    EmitterData *instance = m_pool.get(emitter);
    if (!instance) {
        return;
    }

    instance->position = position;
}

void EmitterPool::set_angle(EmitterHandle emitter, float angle) {
    EmitterData *instance = m_pool.get(emitter);
    if (!instance) {
        return;
    }

    instance->rotation = angle;
}

void EmitterPool::set_emitting(EmitterHandle emitter, bool emitting) {
    EmitterData *instance = m_pool.get(emitter);
    if (!instance) {
        return;
    }

    instance->emitting = emitting;
}
