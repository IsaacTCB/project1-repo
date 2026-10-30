#include "sprite_manager.h"

#include <array>

#include <expected>
#include <glaze/core/context.hpp>
#include <glaze/json.hpp>

#include "core/logger.h"
#include "core/math/vec2.h"
#include "core/math/vec2i.h"
#include "runtime/asset_service.h"

#include "json_meta.h"

static constexpr int MANIFEST_VERSION = 1;

struct SpriteDesc {
    std::string texture;
    Rect rect;
    Vec2 anchor{0.5f, 0.5f};
};

struct SpriteManifestDesc {
    int version;
    std::unordered_map<std::string, std::string> textures;
    std::unordered_map<std::string, SpriteDesc> sprites;
};

static std::expected<void, ManifestLoadError> parse_manifest_file(SpriteManifestDesc &manifest, const std::filesystem::path &filepath) {
    static constexpr glz::opts kManifestOpts{
        .error_on_missing_keys = true,
    };
    std::string json;
    if (auto err = glz::read_file_jsonc<kManifestOpts>(manifest, filepath.string(), json)) {
        LOG_ERROR("manifest '{}' error: {}", filepath.generic_string(),
            glz::format_error(err, json));
        if (err.ec == glz::error_code::file_open_failure) {
            return std::unexpected{ManifestLoadError::NOT_FOUND};
        }
        return std::unexpected{ManifestLoadError::PARSE_ERROR};
    }
    return {};
}

std::expected<void, ManifestLoadError> SpriteManager::load_manifest(const std::filesystem::path &filepath) {
    SpriteManifestDesc manifest;
    if (!parse_manifest_file(manifest, filepath)) {
        return std::unexpected{ManifestLoadError::ERROR};
    }

    if (manifest.version != MANIFEST_VERSION) {
        LOG_ERROR("unsupported manifest version: expected {}, got {}",
            MANIFEST_VERSION, manifest.version);
        return std::unexpected{ManifestLoadError::INVALID_VERSION};
    }

    std::unordered_map<std::string, TextureRef> texture_map;
    std::unordered_map<std::string, Sprite2D> sprite_map;

    /* load all textures from texture map */
    for (const auto &[name, relative_path] : manifest.textures) {
        const auto texture_path = filepath.parent_path() / relative_path;
        TextureRef texture = m_service.get_texture(texture_path.generic_string());
        if (!texture) {
            LOG_ERROR(
                "manifest load error: failed to load sprite texture '{}' at '{}'",
                name, texture_path.generic_string());
            return std::unexpected{ManifestLoadError::ERROR};
        }
        texture_map.emplace(name, std::move(texture));
    }

    /* load sprites */
    for (const auto &[name, sprite_desc] : manifest.sprites) {
        auto it = texture_map.find(sprite_desc.texture);
        if (it == texture_map.end()) {
            LOG_ERROR(
                "manifest load error: sprite '{}' uses unknown texture '{}'",
                name, sprite_desc.texture);
            return std::unexpected{ManifestLoadError::ERROR};
        }

        const TextureRef &texture = it->second;
        const auto [x, y, w, h] = sprite_desc.rect;
        if (w <= 0 || h <= 0 ||
            x < 0 || y < 0 ||
            w > texture.width() - x ||
            h > texture.height() - y) {
            LOG_ERROR("manifest load error: sprite '{}' uses invalid rect dimensions");
            return std::unexpected{ManifestLoadError::ERROR};
        }

        Sprite2D spr{
            .texture = texture,
            .uv_min = Vec2{static_cast<float>(x), static_cast<float>(y)} / Vec2{texture.size()},
            .uv_max = Vec2{static_cast<float>(x + w), static_cast<float>(y + h)} / Vec2{texture.size()},
            .anchor = sprite_desc.anchor,
            .size = Vec2{static_cast<float>(w), static_cast<float>(h)},
        };

        LOG_TRACE("loaded sprite '{}'", name);
        sprite_map.emplace(name, std::move(spr));
    }

    m_sprites = std::move(sprite_map);
    return {};
}

[[nodiscard]] Sprite2D SpriteManager::get_sprite(std::string_view path) const {
    auto it = m_sprites.find(std::string{path});
    if (it == m_sprites.end()) {
        LOG_WARN("sprite '{}' does not exist", path);
        return Sprite2D{};
    }
    return it->second;
}
