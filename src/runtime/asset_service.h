#pragma once

#include <memory>
#include <string_view>

#include "runtime/audio/audio_handles.h"
#include "runtime/renderer/render_handles.h"

#include "asset_ref.h"

using TextureRef = AssetRef<TextureHandle>;
using AudioSourceRef = AssetRef<AudioSourceHandle>;

class ImageData;
class IAudioSource;

class AssetService {
public:
    class Provider {
    public:
        struct RendererProvider {
            using LoadFn = TextureHandle (*)(ImageData &image, void *userdata);
            LoadFn load_texture = nullptr;

            using UnloadFn = void (*)(TextureHandle handle, void *userdata);
            UnloadFn unload_texture = nullptr;

            void *userdata = nullptr;
        } renderer{};

        struct AudioProvider {
            using LoadFn = AudioSourceHandle (*)(std::shared_ptr<IAudioSource> &source, void *userdata);
            LoadFn load_source = nullptr;

            using UnloadFn = void (*)(AudioSourceHandle handle, void *userdata);
            UnloadFn unload_source = nullptr;

            using GetAudioSourceFromFileFn = std::shared_ptr<IAudioSource> (*)(const std::string_view path, void *userdata);
            GetAudioSourceFromFileFn get_audio_source_from_file = nullptr;

            void *userdata = nullptr;
        } audio{};
    };

public:
    explicit AssetService();
    explicit AssetService(const ::AssetService::Provider &provider);
    ~AssetService();

    /* TODO: implement functionality for this. */
    void clear();

    TextureRef get_texture(std::string_view path);
    AudioSourceRef get_audio_source(std::string_view path);

private:
    [[nodiscard]] TextureHandle load_texture(std::string_view path) const;
    void unload_texture(TextureHandle handle) const;

    [[nodiscard]] AudioSourceHandle load_audio_source(std::string_view path) const;
    void unload_audio_source(AudioSourceHandle handle) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    const Provider m_provider;
};
