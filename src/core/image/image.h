#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

enum class PixelFormat : uint8_t {
    INVALID,
    RGB8,
    RGBA8,
    BGR8,
    BGRA8,
};

class ImageData {
public:
    size_t width, height;
    PixelFormat format;
    std::vector<uint8_t> pixels;

    [[nodiscard]]
    size_t bpp() const {
        switch (format) {
            case PixelFormat::RGB8:
            case PixelFormat::BGR8:
                return 3;

            case PixelFormat::RGBA8:
            case PixelFormat::BGRA8:
                return 4;

            case PixelFormat::INVALID:
            default:
                return 0;
        }
    }

    [[nodiscard]]
    size_t row_pitch() const {
        /* TODO: update this function when
         * we add arbitrary stride support */
        return width * bpp();
    }

public:
    [[nodiscard]]
    bool valid() const {
        if (format == PixelFormat::INVALID || width == 0 || height == 0) {
            return false;
        }

        const size_t pitch = row_pitch();
        if (pitch == 0 || pitch < width * bpp()) {
            return false;
        }

        return pixels.size() >= pitch * height;
    }

    operator bool() const {
        return this->valid();
    }
};
