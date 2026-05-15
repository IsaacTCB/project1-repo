#include "decoder.h"
#include "core/image/image.h"

#include <cstdint>

#include <stb_image.h>

ImageData Image::decode(std::string_view path) {
    int width, height, channels;

    stbi_uc *data = stbi_load(path.data(), &width, &height, &channels, 0);
    if (!data) {
        return ImageData{};
    }

    size_t size = width * height * channels;
    std::vector<uint8_t> pixels{data, data + size};
    stbi_image_free(data);

    return ImageData{
        .pixels = std::move(pixels),
        .width = static_cast<size_t>(width),
        .height = static_cast<size_t>(height),
        .channels = static_cast<uint8_t>(channels),
    };
}
