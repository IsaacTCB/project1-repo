#include "decoder.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>

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

static std::optional<ImageData> load_image(std::FILE *file, [[maybe_unused]] std::string_view path) {
    int width = 0, height = 0, channels = 0;

    if (!stbi_info_from_file(file, &width, &height, &channels)) {
        return std::nullopt;
    }

    ASSERT(width > 0, "Decoded image width must be >0");
    ASSERT(height > 0, "Decoded image height must be >0");
    ASSERT(channels > 0, "Decoded image channels must be >0");

    auto pixel_format = convert_stbi_channels_to_pixelformat(channels);
    if (pixel_format == PixelFormat::INVALID) {
#if DEBUG
        LOG_WARN(
            "Image at '{0}' uses an unsupported channel count "
            "of {1}; decoding it into PixelFormat::RGBA8...",
            path, channels);
#endif

        /* in case the pixel format is unsupported, we simply
         * just re-encode the channel count to RGBA8. */
        pixel_format = PixelFormat::RGBA8;
        channels = 4;
    }

    int temp = 0;
    stbi_uc *data = stbi_load_from_file(file, &width, &height, &temp, channels);
    if (data) {
        /* copy the stb buffer into a C++ vector buffer */
        auto size = static_cast<size_t>(width * height * channels);
        std::vector<uint8_t> pixels{data, data + size};
        stbi_image_free(data); /* we no longer need the stbi image */

        return ImageData{
            .width = static_cast<size_t>(width),
            .height = static_cast<size_t>(height),
            .format = pixel_format,
            .pixels = std::move(pixels),
        };
    }

    return std::nullopt;
}

ImageData Image::decode(std::string_view path) {
    std::FILE *file = std::fopen(path.data(), "rb");
    if (!file) {
        return ImageData{};
    }

    auto image = load_image(file, path);
    std::fclose(file);

    return image ? *image : ImageData{};
}
