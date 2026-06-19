#include "pspgu_texture.h"

#include <cstdint>
#include <cstdlib>
#include <pspgu.h>

#include "core/assertion.h"

static size_t get_format_bpp(int format) {
    switch (format) {
        case GU_PSM_5650:
        case GU_PSM_5551:
        case GU_PSM_4444:
            return 2;
        case GU_PSM_8888:
            return 4;
        default:
            return 0;
    }
}

static uint16_t pow_of_2(uint16_t value) {
    if (value == 0) {
        return 0;
    }

    uint16_t pow = 2;
    while (pow < value) {
        pow *= 2;
    }

    return pow;
}

void PSPGuImageBuffer::allocate() {
    ASSERT(m_ptr == nullptr, "PSPGuImageBuffer::allocate(): Attempted to reallocate with an image value!")
    ASSERT(width <= 512, "PSPGuImageBuffer::allocate(): Width value must be a power of 2");
    ASSERT(height <= 512, "PSPGuImageBuffer::allocate(): Height value must be a power of 2");
    ASSERT(pow_of_2(width) == width, "PSPGuImageBuffer::allocate(): Width value must be a power of 2");
    ASSERT(pow_of_2(height) == height, "PSPGuImageBuffer::allocate(): Height value must be a power of 2");

    m_size = width * height * get_format_bpp(format);
    ASSERT(m_size > 0, "PSPGuImageBuffer::allocate(): Size must be bigger than 0");

    m_ptr = static_cast<uint8_t *>(aligned_alloc(m_size, 64));
    if (!m_ptr) {
        ASSERT("allocation failed");
        m_size = 0;
    }
}

void PSPGuImageBuffer::free() {
    if (m_ptr) {
        ::free(m_ptr);
        m_ptr = nullptr;
        m_size = 0;
    }
}

PSPGuImageBuffer convert_image(const ImageData &image) {
    PSPGuImageBuffer buffer{
        pow_of_2(image.width),
        pow_of_2(image.height),
        GU_PSM_8888,
    };

    uint8_t *dst = buffer.data();
    const uint8_t *src = image.pixels.data();

    size_t dst_pitch = buffer.width * get_format_bpp(buffer.format);
    size_t src_pitch = image.width * image.channels;
    for (size_t y = 0; y < image.height; ++y) {
        for (size_t x = 0; x < src_pitch; ++x) {
            dst[x + (y * dst_pitch)] = src[x + (y * src_pitch)];
        }
    }

    return buffer;
}
