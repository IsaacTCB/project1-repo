#include "particles_emitter.h"

#include "core/math/vec2.h"

#include "particles.h"

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
