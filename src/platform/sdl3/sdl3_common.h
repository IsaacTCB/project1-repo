#pragma once

#include <SDL3/SDL.h>

struct PlatformImplState {
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
};