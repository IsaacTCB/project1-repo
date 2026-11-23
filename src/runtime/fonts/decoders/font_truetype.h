#pragma once

#include <cstdint>
#include <vector>

#include "runtime/fonts/font_resource.h"
#include "runtime/renderer/render_handles.h"

struct FontLoadDesc {
    float pixel_height = 16.0f;
    uint8_t oversampling = 2;
    int atlas_width = 512;
    int atlas_height = 512;
};

struct FontLoader {
public:
    struct LoadTextureCallback {
        using LoadTextureFn = TextureHandle (*)(std::vector<uint8_t> bitmap, int width, int height, void *userdata);
        LoadTextureFn callback = nullptr;
        void *userdata = nullptr;
    };

public:
    explicit FontLoader() = default;
    explicit FontLoader(const LoadTextureCallback &texture_load)
        : m_texture(texture_load) {}

    void set_callbacks(const LoadTextureCallback &callback) {
        m_texture = callback;
    }

    std::optional<FontResource> load_truetype(std::vector<uint8_t> file_data, const FontLoadDesc &opts = {});

private:
    LoadTextureCallback m_texture;
};
