#pragma once

#include "runtime/asset_service.h"
#include "runtime/renderer/sprite_2d.h"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

enum class ManifestLoadError : uint8_t {
    ERROR = 0, /* generic error */
    NOT_FOUND,
    PARSE_ERROR,
    INVALID_VERSION,
};

class SpriteManager {
public:
    SpriteManager(AssetService &service)
        : m_service(service) {};

    std::expected<void, ManifestLoadError> load_manifest(const std::filesystem::path &filepath);

    [[nodiscard]] Sprite2D get_sprite(std::string_view path) const;

private:
    std::expected<void, ManifestLoadError> add_manifest_group(const std::filesystem::path &filepath);

private:
    std::unordered_map<std::string, Sprite2D> m_sprites;
    AssetService &m_service;
};
