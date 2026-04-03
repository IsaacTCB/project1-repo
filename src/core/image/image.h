#pragma once

#include <cstdint>
#include <vector>

class ImageData {
public:
    std::vector<uint8_t> pixels;
    size_t width;
    size_t height;
    uint8_t channels;

public:
    operator bool() const {
        return (pixels.size() > 0 ||
            width > 0 ||
            height > 0 ||
            channels > 0);
    }
};
