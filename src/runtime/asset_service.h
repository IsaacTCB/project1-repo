#pragma once

#include <memory>
#include <string_view>

#include "renderer/api/render_handles.h"

class ImageData;
class AssetService;

class TextureRef {
public:
    TextureRef()
        : m_ptr(nullptr) {}
    ~TextureRef();

    TextureRef(const TextureRef &other);
    TextureRef(TextureRef &&other) noexcept;
    TextureRef &operator=(const TextureRef &rhs);
    TextureRef &operator=(TextureRef &&rhs) noexcept;

    void reset();
    void acquire();
    void release();

    [[nodiscard]] TextureHandle get() const;
    [[nodiscard]] operator TextureHandle() const { return get(); }
    [[nodiscard]] operator bool() const { return m_ptr != nullptr; }

protected:
    explicit TextureRef(void *ptr)
        : m_ptr(ptr) {}

private:
    friend AssetService;
    void *m_ptr;
};

class AssetService {
public:
    class Provider {
    public:
        using LoadFn = TextureHandle (*)(ImageData &image, void *userdata);
        using UnloadFn = void (*)(TextureHandle handle, void *userdata);

        struct {
            LoadFn load_texture;
            UnloadFn unload_texture;
            void *userdata;
        } renderer;
    };

public:
    explicit AssetService() = delete;
    explicit AssetService(const ::AssetService::Provider &provider);
    ~AssetService();

    void clear();
    TextureRef get_texture(std::string_view path);

public:
    [[nodiscard]] TextureHandle load_texture(std::string_view path) const;
    void unload_texture(TextureHandle handle) const;
    void remove_texture(void *slot);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    const Provider m_provider;

private:
    friend TextureRef;
};
