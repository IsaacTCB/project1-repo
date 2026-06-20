#pragma once

#include <algorithm>
#include <cstdint>
#include <pspgu.h>

#include "core/image/image.h"
#include "core/math/vec2.h"

class PSPGuImageBuffer {
public:
    explicit PSPGuImageBuffer(uint16_t width, uint16_t height, uint32_t format)
        : width(width), height(height), format(format) {
        allocate();
    }

    ~PSPGuImageBuffer() {
        release();
    }

    PSPGuImageBuffer(const PSPGuImageBuffer &other)
        : width(other.width), height(other.height),
          format(other.format), m_size(other.m_size) {
        allocate();
        if (m_ptr) {
            std::copy(other.m_ptr, other.m_ptr + m_size, m_ptr);
        }
    }

    PSPGuImageBuffer(PSPGuImageBuffer &&rhs) noexcept
        : width(rhs.width), height(rhs.height),
          format(rhs.format), m_ptr(rhs.m_ptr), m_size(rhs.m_size) {
        rhs.m_ptr = nullptr;
        rhs.m_size = 0;
    }

    PSPGuImageBuffer &operator=(const PSPGuImageBuffer &other) {
        if (&other == this) {
            return *this;
        }

        release();

        width = other.width;
        height = other.height;
        format = other.format;
        m_ptr = nullptr;
        m_size = 0;

        if (other.m_ptr) {
            allocate();
            if (m_ptr) {
                std::copy(other.m_ptr, other.m_ptr + m_size, m_ptr);
            }
        }

        return *this;
    }

    PSPGuImageBuffer &operator=(PSPGuImageBuffer &&other) noexcept {
        if (&other == this) {
            return *this;
        }

        release();

        width = other.width;
        height = other.height;
        format = other.format;
        m_ptr = other.m_ptr;
        m_size = other.m_size;

        other.m_ptr = nullptr;
        other.m_size = 0;

        return *this;
    }

    [[nodiscard]] uint8_t *data() const { return m_ptr; }

private:
    void allocate();
    void release();

public:
    uint16_t width;
    uint16_t height;
    uint32_t format;

private:
    uint8_t *m_ptr{};
    size_t m_size{};
};

struct PSPGuTexture {
    PSPGuImageBuffer buffer;
    Vec2 logical_ratio;
    uint16_t logical_width;
    uint16_t logical_height;
    bool filter;
};

PSPGuImageBuffer convert_image(const ImageData &image);
void pspgu_texture_bind(PSPGuImageBuffer &buffer);
void pspgu_texture_unbind();
