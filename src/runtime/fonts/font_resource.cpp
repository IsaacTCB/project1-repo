#include "font_resource.h"

#include "core/logger.h"
#include "core/math/vec2i.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <optional>
#include <vector>

#include <stb_truetype.h>

namespace {

constexpr int FIRST_CHAR = 32;
constexpr int CHAR_COUNT = 95;

constexpr size_t ATLAS_WIDTH = 512;
constexpr size_t ATLAS_HEIGHT = 512;
constexpr Vec2i ATLAS_DIMENSIONS = Vec2i{ATLAS_WIDTH, ATLAS_HEIGHT};

FontResource::LoadTextureFn s_load_callback = nullptr;
void *s_load_userdata;

std::optional<std::vector<uint8_t>> load_file(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> file_data(size);
    file.read(
        reinterpret_cast<char *>(file_data.data()),
        file_data.size());

    return file_data;
}

}; // namespace

void FontResource::set_load_texture_callback(
    FontResource::LoadTextureFn callback, void *userdata) {
    s_load_callback = callback;
    s_load_userdata = userdata;
}

std::optional<FontResource> FontResource::load_font(const std::filesystem::path &path, float pixel_height) {
    const auto file_data = load_file(path);
    if (!file_data) {
        LOG_TRACE("failed to locate file at '{}'", path.generic_string());
        return std::nullopt;
    }

    stbtt_fontinfo info{};
    if (!stbtt_InitFont(&info, file_data->data(), 0)) {
        LOG_TRACE("failed to init font '{}'", path.generic_string());
        return std::nullopt;
    }

    /* 1. generate glyph atlas image */
    std::vector<uint8_t> pixels(
        ATLAS_WIDTH * ATLAS_HEIGHT, 0);

    stbtt_pack_context pack{};
    if (!stbtt_PackBegin(
            &pack, pixels.data(),
            ATLAS_WIDTH, ATLAS_HEIGHT,
            0, 2, nullptr)) {
        LOG_TRACE("failed to being packing font '{}'", path.generic_string());
        return std::nullopt;
    }
    stbtt_PackSetOversampling(&pack, 2, 2);

    std::array<stbtt_packedchar, CHAR_COUNT> packed{};
    if (!stbtt_PackFontRange(
            &pack, file_data->data(),
            0, pixel_height,
            FIRST_CHAR, CHAR_COUNT,
            packed.data())) {
        LOG_TRACE("failed to packed all fonts '{}'", path.generic_string());
        stbtt_PackEnd(&pack);
        return std::nullopt;
    }
    stbtt_PackEnd(&pack);

    FontResource result;

    /* 2. calculate metrics from text info */
    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);
    const float scale = stbtt_ScaleForPixelHeight(&info, pixel_height);
    result.metrics = {
        .ascent = static_cast<float>(ascent) * scale,
        .descent = static_cast<float>(descent) * scale,
        .line_gap = static_cast<float>(line_gap) * scale,
    };

    /* 3. convert stbtt glyphs into proper Glyphs */
    for (int i = 0; i < CHAR_COUNT; ++i) {
        const auto &g = packed[i];
        const Glyph glyph{
            .uv_min =
                Vec2{static_cast<float>(g.x0), static_cast<float>(g.y0)} / Vec2{ATLAS_DIMENSIONS},
            .uv_max =
                Vec2{static_cast<float>(g.x1), static_cast<float>(g.y1)} / Vec2{ATLAS_DIMENSIONS},

            .size = Vec2{
                g.xoff2 - g.xoff,
                g.yoff2 - g.yoff,
            },
            .bearing = Vec2{
                g.xoff,
                g.yoff,
            },
            .advance = g.xadvance,
        };
        result.add_glyph(
            static_cast<Codepoint>(FIRST_CHAR + i),
            glyph);

        /* get kerning values */
        for (int j = 0; j < CHAR_COUNT; ++j) {
            int kern = stbtt_GetCodepointKernAdvance(&info, i, j);
            if (kern == 0) {
                continue;
            }
            result.set_kerning(i, j, static_cast<float>(kern) * scale);
        }
    }

    /* 4. submit atlas glyph to gpu (temporary) */
    result.atlas = s_load_callback(
        std::move(pixels),
        static_cast<int>(ATLAS_WIDTH),
        static_cast<int>(ATLAS_HEIGHT),
        s_load_userdata);
    if (!result.atlas) {
        LOG_TRACE("failed to submit font '{}'", path.generic_string());
        return std::nullopt;
    }

    return result;
}
