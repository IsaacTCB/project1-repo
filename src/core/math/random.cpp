#include "random.h"
#include "core/assertion.h"

#include <chrono>
#include <cstdint>

#if defined(ENABLE_PCG32)
#include <pcg_random.hpp>
using RandomAlgorithm = pcg32;
#else
#include <random>
using RandomAlgorithm = std::mt19937;
#endif


struct RandomImpl {
public:
    RandomAlgorithm rng;

public:
    uint32_t initial_seed;
};

static RandomImpl s_impl{};

/* ================ */
/*       Seed       */
/* ================ */

void Random::set_seed(uint32_t seed) {
    s_impl.initial_seed = seed;
    s_impl.rng.seed(s_impl.initial_seed);
}

uint32_t Random::get_seed() {
    return s_impl.initial_seed;
}

uint32_t Random::randomize() {
    /* TODO: use platform abstraction
     * layer to get system clock */
    auto time = static_cast<uint64_t>(
        std::chrono::steady_clock::now()
            .time_since_epoch()
            .count());

    /* fold 64 bits into 32 bits and upload seed */
    auto seed = static_cast<uint32_t>(time ^ (time >> 32));
    set_seed(seed);

    return seed;
}

/* ================ */
/*     Integers     */
/* ================ */

uint32_t Random::randi() {
    return static_cast<uint32_t>(s_impl.rng());
}

int Random::randi_range(int min, int max) {
    ASSERT(min < max, "Random::randi_range(): min is larger than max.");

    /* add one, cuz modulo is exclusive */
    auto length = static_cast<uint32_t>(
        static_cast<int64_t>(max) - min + 1);

    uint32_t rand;
#if defined(ENABLE_PCG32)
    rand = static_cast<uint32_t>(s_impl.rng(length));
#else
    /* loses uniform distribution, but it's faster. */
    rand = static_cast<uint32_t>(s_impl.rng());
    rand %= length;
#endif

    return static_cast<int>(min + rand);
}

/* ================ */
/*      Floats      */
/* ================ */

float Random::randf() {
    constexpr auto RANDOM_MAX = static_cast<float>(
        RandomAlgorithm::max());

    /* normalize length */
    return static_cast<float>(s_impl.rng()) /
           static_cast<float>(RANDOM_MAX);
}

float Random::randf_range(float min, float max) {
    ASSERT(min < max, "Random::randf_range(): min is larger than max.");
    return ((max - min) * Random::randf()) + min;
}
