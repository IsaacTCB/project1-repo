#pragma once

#include <cstdint>

namespace Random {

/**
 * @brief Updates PRNG seed value
 *
 * @param seed A 32-bit value to set state
 */
void set_seed(uint32_t seed);

/**
 * @brief Returns current seed value
 *
 * @return Last set seed value
 */
uint32_t get_seed();

/**
 * @brief Generates and applies a "random"
 * seed based on the current system clock.
 *
 * Calling this function on startup ensures
 * that the PRNG produces different values
 * across multiple sessions.
 *
 * @return The newly generated 32-bit seed
 */
uint32_t randomize();

/**
 * @brief Generates a pseudo-random integer value
 *
 * @return Random integer between 0 and 2^32-1 inclusive
 */
uint32_t randi();

/**
 * @brief Generates a pseudo-random integer value inside the specified range.
 *
 * @param min Minimum range value. Must be smaller than max.
 * @param max Maximum range value. Must be bigger than min.
 * @return Random integer between `min` and `max` inclusive.
 */
int randi_range(int min, int max);

/**
 * @brief Generates a pseudo-random floating point value
 *
 * @return Random floating point number between 0.0 and 1.0 inclusive.
 */
float randf();

/**
 * @brief Generates a pseudo-random floating point value inside the specified range.
 *
 * @param min Minimum range value. Must be smaller than max.
 * @param max Maximum range value. Must be bigger than min.
 * @return Random floating point number between `min` and `max` inclusive.
 */
float randf_range(float min, float max);

}; // namespace Random
