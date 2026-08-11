#pragma once

#include <cstddef>

#include "gl_shader.h"

struct GLTexture {
    GLuint id;
    size_t width;
    size_t height;
};
