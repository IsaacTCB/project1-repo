#pragma once

#include "core/math/vec2.h"
#include "core/math/vec2i.h"
#include "runtime/asset_service.h"
#include "runtime/renderer/sprite_2d.h"

#include <string>
#include <string_view>
#include <unordered_map>

class SpriteManager {
public:
    SpriteManager(AssetService &service)
        : m_service(service) {};

    Sprite2D load_sprite(std::string_view path, Vec2i region_min, Vec2i region_max, Vec2 anchor = {}) {
        auto t = find_or_allocate_texture(std::string{path});
        if (!t) {
            LOG_WARN("failed to load sprite");
            return {};
        }

        const auto size = Vec2{t.size()};
        return {
            .texture = std::move(t),
            .uv_min = Vec2{region_min} / size,
            .uv_max = Vec2{region_max} / size,
            .anchor = anchor,
            .size = Vec2{region_max - region_min},
        };
    }

private:
    TextureRef find_or_allocate_texture(const std::string &path) {
        auto it = m_textures.find(path);
        if (it == m_textures.end()) {
            auto t = m_service.get_texture(path);
            auto [emplaced, success] = m_textures.try_emplace(path, std::move(t));
            it = emplaced;
        }
        return it->second;
    }

private:
    std::unordered_map<std::string, TextureRef> m_textures;
    AssetService &m_service;
};
