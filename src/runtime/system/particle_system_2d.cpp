#include "particle_system_2d.h"

#include "core/math/random.h"
#include "core/math/vec2.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

/**
 * @brief Lerps a color from A to B
 *
 * @param a color A
 * @param b color B
 * @param t amount to lerp to
 * @return A new color object interpolated between `a`` and `b` using `t`
 */
static Color lerp_color(const Color &a, const Color &b, float t) {
    return Color{
        static_cast<uint8_t>(a.r + static_cast<int>(static_cast<float>(b.r - a.r) * t)),
        static_cast<uint8_t>(a.g + static_cast<int>(static_cast<float>(b.g - a.g) * t)),
        static_cast<uint8_t>(a.b + static_cast<int>(static_cast<float>(b.b - a.b) * t)),
        static_cast<uint8_t>(a.a + static_cast<int>(static_cast<float>(b.a - a.a) * t)),
    };
}

ParticleSystem2D::ParticleSystem2D() {
    m_particles = std::make_unique<ParticleStorage>();
}

void ParticleSystem2D::update(float delta) {
    update_emitters(delta);
    update_particles(delta);
}

void ParticleSystem2D::render(RenderQueue &queue) {
    for (size_t p{}; p < PARTICLE_COUNT; ++p) {
        if (!m_particles->alive[p]) {
            continue;
        }

        ParticleEffect2D *effect = m_effects.get(m_particles->effect[p]);
        if (!effect) {
            continue;
        }

        /* get lifetime percent */
        float t = m_particles->lifetime_remaining[p] /
                  m_particles->lifetime[p];
        t = 1.0f - std::clamp(t, 0.0f, 1.0f);

        /* get effect values */
        Color color = lerp_color(effect->color_start, effect->color_end, t);
        float scale = std::lerp(effect->scale_start, effect->scale_end, t);

        /* draw particle (centered) */
        Vec2 size = effect->particle_size * scale;
        Vec2 pos = m_particles->pos[p] - (size * 0.5f);
        queue.draw_color(color, Vec2i{pos}, Vec2i{size});
    }
}

/* private */

void ParticleSystem2D::update_emitters(float delta) {
    m_emitters.update(
        delta,
        [](EmitterPool::EmitterData &instance, void *user) {
            auto *system = static_cast<ParticleSystem2D *>(user);
            system->emit(instance);
        },
        this);
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

void ParticleSystem2D::emit(EmitterPool::EmitterData &emitter) {
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
    m_particles->effect[m_index] = emitter.effect;

    /* apply emitter state */
    m_particles->pos[m_index] = emitter.position;
    m_particles->alive[m_index] = true;

    m_index = (m_index + 1) % PARTICLE_COUNT;
}
