#pragma once

#include <cstddef>

#include <glad/gl.h>

struct GLTexture {
    GLuint id;
    size_t width;
    size_t height;
};
