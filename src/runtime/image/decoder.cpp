#include "decoder.h"

#include "core/assertion.h"
#include "core/image/image.h"

#include <cstdint>

#include <stb_image.h>

ImageData Image::decode(std::string_view path) {
    int width, height, channels;

    stbi_uc *data = stbi_load(path.data(), &width, &height, &channels, 0);
    if (!data) {
        return ImageData{};
    }

    ASSERT(width > 0, "Decoded image width must be >0");
    ASSERT(height > 0, "Decoded image height must be >0");
    ASSERT(channels > 0, "Decoded image channels must be >0");

    auto size = static_cast<size_t>(width * height * channels);
    std::vector<uint8_t> pixels{data, data + size};
    stbi_image_free(data);

    return ImageData{
        .pixels = std::move(pixels),
        .width = static_cast<size_t>(width),
        .height = static_cast<size_t>(height),
        .channels = static_cast<uint8_t>(channels),
    };
}
