#pragma once

#include <cstddef>
#include <pspgu.h>

using vram_ptr = void *;

/* PSP screen dimensions */
inline constexpr int PSP_SCREEN_WIDTH = 480;
inline constexpr int PSP_SCREEN_HEIGHT = 272;

/* PSP framebuffer & backbuffer */
inline constexpr int PSP_FRAMEBUFFER_WIDTH = 512;
inline constexpr int PSP_FRAMEBUFFER_HEIGHT = 272;
inline constexpr uint32_t PSP_FRAMEBUFFER_PSM = GU_PSM_8888;

/* display list */
inline constexpr size_t PSP_DISPLAYLIST_SIZE = 0x20000;
