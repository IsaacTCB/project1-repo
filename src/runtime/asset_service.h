#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

#include "core/image/image.h"
#include "core/math/vec2i.h"
#include "runtime/audio/audio_handles.h"
#include "runtime/renderer/render_handles.h"

#include "asset_ref.h"

/* TODO: move asset definitions outta here */
struct TextureAsset {
    TextureHandle handle;
    PixelFormat format;
    Vec2i size;
    uint64_t native_handle; /* opaque backend handle */

    operator TextureHandle() const { return handle; }
    operator bool() const { return handle.valid(); }
};

class TextureRef : public AssetRefMixin<TextureRef, TextureAsset> {
public:
    using AssetRefMixin::AssetRefMixin;

    /* texture metadata */
    [[nodiscard]] TextureHandle handle() const { return this->get().handle; }
    [[nodiscard]] PixelFormat format() const { return this->get().format; }
    [[nodiscard]] Vec2i size() const { return this->get().size; };
    [[nodiscard]] int width() const { return this->get().size.x; };
    [[nodiscard]] int height() const { return this->get().size.y; };
    [[nodiscard]] uint64_t native() const { return this->get().native_handle; }

    /* misc */
    [[nodiscard]] bool valid() const { return static_cast<bool>(this->get()); }
    [[nodiscard]] operator bool() const { return this->valid(); }
};

struct AudioSourceAsset {
    AudioSourceHandle handle;
    uint64_t frame_count;
    uint32_t sample_rate;
    uint8_t channels;

    operator AudioSourceHandle() const { return handle; }
    operator bool() const { return handle.valid(); }
};

class AudioSourceRef : public AssetRefMixin<AudioSourceRef, AudioSourceAsset> {
public:
    using AssetRefMixin::AssetRefMixin;

    /* audio source metadata */
    [[nodiscard]] AudioSourceHandle handle() const { return this->get().handle; }
    [[nodiscard]] uint64_t frame_count() const { return this->get().frame_count; }
    [[nodiscard]] uint32_t sample_rate() const { return this->get().sample_rate; }
    [[nodiscard]] uint8_t channels() const { return this->get().channels; }

    /* misc */
    [[nodiscard]] bool valid() const { return static_cast<bool>(this->get()); }
    [[nodiscard]] operator bool() const { return this->valid(); }
};

class ImageData;
class IAudioSource;

class AssetService {
public:
    class Provider {
    public:
        struct RendererProvider {
            using LoadFn = TextureAsset (*)(ImageData &image, void *userdata);
            LoadFn load_texture = nullptr;

            using UnloadFn = void (*)(TextureHandle handle, void *userdata);
            UnloadFn unload_texture = nullptr;

            void *userdata = nullptr;
        } renderer{};

        struct AudioProvider {
            using LoadFn = AudioSourceAsset (*)(std::shared_ptr<IAudioSource> &source, void *userdata);
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
    [[nodiscard]] TextureAsset load_texture(std::string_view path) const;
    void unload_texture(const TextureAsset &texture) const;

    [[nodiscard]] AudioSourceAsset load_audio_source(std::string_view path) const;
    void unload_audio_source(const AudioSourceAsset &source) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    const Provider m_provider;
};
