#include "font_truetype.h"

#include "core/assertion.h"
#include "core/math/vec2i.h"
#include "runtime/fonts/font_resource.h"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include <stb_truetype.h>

std::optional<FontResource> FontLoader::load_truetype(std::vector<uint8_t> file_data, const FontLoadDesc &opts) {
    ASSERT(m_texture.callback, "FontLoader callbacks was not setup");

    constexpr auto FIRST_CHAR = FontResource::FIRST_CHAR;
    constexpr auto CHAR_COUNT = FontResource::CHAR_COUNT;

    stbtt_fontinfo info{};
    if (!stbtt_InitFont(&info, file_data.data(), 0)) {
        return std::nullopt;
    }

    /* 1. generate glyph atlas image */
    std::vector<uint8_t> pixels(
        opts.atlas_width * opts.atlas_height, 0);

    stbtt_pack_context pack{};
    if (!stbtt_PackBegin(
            &pack, pixels.data(),
            opts.atlas_width, opts.atlas_height,
            0, 2, nullptr)) {
        return std::nullopt;
    }
    stbtt_PackSetOversampling(&pack, opts.oversampling, opts.oversampling);

    std::array<stbtt_packedchar, FontResource::CHAR_COUNT> packed{};
    if (!stbtt_PackFontRange(
            &pack, file_data.data(),
            0, opts.pixel_height,
            FontResource::FIRST_CHAR, FontResource::CHAR_COUNT,
            packed.data())) {
        stbtt_PackEnd(&pack);
        return std::nullopt;
    }
    stbtt_PackEnd(&pack);

    FontResource result;

    /* 2. calculate metrics from text info */
    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);
    const float scale = stbtt_ScaleForPixelHeight(&info, opts.pixel_height);
    result.metrics = {
        .ascent = static_cast<float>(ascent) * scale,
        .descent = static_cast<float>(descent) * scale,
        .line_gap = static_cast<float>(line_gap) * scale,
    };

    /* 3. convert stbtt glyphs into proper Glyphs */
    const Vec2 atlas_size = Vec2{
        static_cast<float>(opts.atlas_width),
        static_cast<float>(opts.atlas_height),
    };
    for (int i = 0; i < CHAR_COUNT; ++i) {
        const auto &g = packed[i];
        const FontResource::Glyph glyph{
            .uv_min =
                Vec2{static_cast<float>(g.x0), static_cast<float>(g.y0)} / atlas_size,
            .uv_max =
                Vec2{static_cast<float>(g.x1), static_cast<float>(g.y1)} / atlas_size,

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
            static_cast<FontResource::Codepoint>(FIRST_CHAR + i),
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
    result.m_atlas = m_texture.callback(
        std::move(pixels),
        static_cast<int>(opts.atlas_width),
        static_cast<int>(opts.atlas_height),
        m_texture.userdata);
    if (!result.valid()) {
        return std::nullopt;
    }

    return result;
}
