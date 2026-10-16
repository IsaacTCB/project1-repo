#include "decoder.h"

#include <cstdint>

#include "core/assertion.h"
#include "core/image/image.h"

#include <stb_image.h>

static PixelFormat convert_stbi_channels_to_pixelformat(int channels) {
    switch (channels) {
        case 3:
            return PixelFormat::RGB8;
        case 4:
            return PixelFormat::RGBA8;
    }
    /* unsupported channel color */
    return PixelFormat::INVALID;
}

ImageData Image::decode(std::string_view path) {
    int width, height, channels;

    stbi_uc *data = stbi_load(path.data(), &width, &height, &channels, 0);
    if (!data) {
        return ImageData{};
    }

    ASSERT(width > 0, "Decoded image width must be >0");
    ASSERT(height > 0, "Decoded image height must be >0");
    ASSERT(channels > 0, "Decoded image channels must be >0");

    auto pixel_format = convert_stbi_channels_to_pixelformat(channels);
    if (pixel_format == PixelFormat::INVALID) {
        stbi_image_free(data);
        return ImageData{};
    }

    auto size = static_cast<size_t>(width * height * channels);
    std::vector<uint8_t> pixels{data, data + size};
    stbi_image_free(data);

    return ImageData{
        .width = static_cast<size_t>(width),
        .height = static_cast<size_t>(height),
        .format = pixel_format,
        .pixels = std::move(pixels),
    };
}
