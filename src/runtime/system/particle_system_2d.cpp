#include "particle_system_2d.h"

#include "core/math/random.h"
#include "core/math/vec2.h"
#include "renderer/api/render_handles.h"

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
 * @return A new color object interpolated between `a` and `b` using `t`
 */
static Color lerp_color(const Color &a, const Color &b, float t) {
    return Color{
        static_cast<uint8_t>(a.r + static_cast<int>(static_cast<float>(b.r - a.r) * t)),
        static_cast<uint8_t>(a.g + static_cast<int>(static_cast<float>(b.g - a.g) * t)),
        static_cast<uint8_t>(a.b + static_cast<int>(static_cast<float>(b.b - a.b) * t)),
        static_cast<uint8_t>(a.a + static_cast<int>(static_cast<float>(b.a - a.a) * t)),
    };
}

/* xorshift32 pseudo-random number generator
 * We use xorshift because it's fast, and
 * we don't need a high-quality rng for this */
static uint32_t xorshift32(uint32_t &state);
static void xorshift32_seed(uint32_t &state, uint32_t seed = 0);
static float xorshift32_randf(uint32_t &state);
static float xorshift32_randf_range(uint32_t &state, float min, float max);

ParticleSystem2D::ParticleSystem2D() {
    m_particles = std::make_unique<ParticleStorage>();

    /* seed stuff */
    seed();
}

void ParticleSystem2D::seed(uint32_t seed) {
    xorshift32_seed(m_seed, seed);
}

void ParticleSystem2D::update(float delta) {
    update_emitters(delta);
    update_particles(delta);
}

void ParticleSystem2D::render(RenderQueue &queue) {
    particle_foreach([&](size_t p) {
        const ParticleEffect2D *effect = m_effects.get(m_particles->effect[p]);
        if (!effect) {
            return;
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

        if (TextureHandle texture = effect->particle_texture.get()) {
            queue.draw_texture_blended(texture, color, Vec2i{pos}, Vec2i{size});
        } else {
            queue.draw_color(color, Vec2i{pos}, Vec2i{size});
        }
    });
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
    particle_foreach([&](size_t p) {
        const ParticleEffect2D *effect = m_effects.get(m_particles->effect[p]);
        if (!effect) {
            m_particles->alive[p] = false;
            return;
        }

        /* apply gravity to velocity */
        Vec2 &vel = m_particles->vel[p];
        vel += effect->particle_gravity;

        /* update position from velocity */
        m_particles->pos[p] += vel * delta;

        /* update lifetime */
        m_particles->lifetime_remaining[p] -= delta;
        if (m_particles->lifetime_remaining[p] <= 0.0f) {
            m_particles->alive[p] = false;
        }
    });
}

void ParticleSystem2D::emit(EmitterPool::EmitterData &emitter) {
    const ParticleEffect2D *effect = m_effects.get(emitter.effect);
    if (!effect) {
        return;
    }

    /* set lifetime */
    const float total_lifetime = xorshift32_randf_range(m_seed,
        effect->lifetime_min, effect->lifetime_max);
    m_particles->lifetime_remaining[m_index] =
        m_particles->lifetime[m_index] = total_lifetime;

    float angle = emitter.rotation +
                  xorshift32_randf_range(m_seed, -effect->emission_spread, effect->emission_spread);

    float initial_speed = xorshift32_randf_range(m_seed, effect->speed_min, effect->speed_max);
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

/* ======================== */
/*   xorshift32 algorithm   */
/* ======================== */

static void xorshift32_seed(uint32_t &state, uint32_t seed) {
    state = seed;

    /* initializes seed (seed must be non-zero,
     * otherwise xorshift32 gets locked up) */
    while (state == 0) {
        state = Random::randi();
    }

    /* seed */
    (void)(xorshift32(state));
}

static uint32_t xorshift32(uint32_t &state) {
    uint32_t x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return state = x;
}

static float xorshift32_randf(uint32_t &state) {
    return static_cast<float>(xorshift32(state) & 0x00FFFFFF) /
           static_cast<float>(0x01000000);
}

static float xorshift32_randf_range(uint32_t &state, float min, float max) {
    return min + ((max - min) * xorshift32_randf(state));
}
