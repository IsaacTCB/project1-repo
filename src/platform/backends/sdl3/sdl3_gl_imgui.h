#pragma once

#include "runtime/debug/imgui_backend.h"

struct SDL_Window;
IImGuiBackend *sdl3_gl_create_imgui(SDL_Window *window);
