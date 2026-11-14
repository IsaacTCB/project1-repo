#pragma once

#include <SDL3/SDL.h>
#include <cstdint>

inline uint32_t g_fps_max = 0;
inline uint64_t g_fps_tickrate = 0;

void log_init();
