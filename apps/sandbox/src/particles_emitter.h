#pragma once

#include "core/utils/handles.h"

using EmitterHandle = Handle;

class Vec2;
class ParticleSystem2D;

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
        m_system = nullptr;
        m_handle = {};
    }

protected:
    friend ParticleSystem2D;
    ParticleEmitter2D(ParticleSystem2D *system, EmitterHandle handle)
        : m_system(system), m_handle(handle) {}

private:
    void destroy_handle();

private:
    ParticleSystem2D *m_system{nullptr};
    EmitterHandle m_handle{};
};
