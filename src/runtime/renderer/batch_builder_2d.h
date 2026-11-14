#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "core/math/color.h"
#include "core/math/vec2.h"
#include "core/math/vec3.h"

#include "graphics/api/render_device.h"
#include "runtime/renderer/render_handles.h"

struct DrawRectDesc {
private:
    /* static vec2s to allow for default
     * instantiation of some of the
     * references below */
    static inline Vec2 s_vec2_zero{0.0f, 0.0f};
    static inline Vec2 s_vec2_one{1.0f, 1.0f};

public:
    const Vec2 &pos;
    const Vec2 &size;
    const Vec2 &anchor = s_vec2_zero;
    float rotation = 0.0f;

    const Vec2 &uv_min = s_vec2_zero;
    const Vec2 &uv_max = s_vec2_one;

    Color color{};
    TextureHandle texture{};
    bool filter = true;
};

class BatchBuilder2D {
public:
    BatchBuilder2D(IRenderDevice *rd)
        : m_rd(rd) {};

    void init(size_t quad_count = 1000);
    void close();

    void draw_rect(const DrawRectDesc &desc);
    void flush();

private:
    void push_quad_indices(size_t index_base);

private:
    struct BatchKey {
        TextureHandle texture;
        SamplerHandle sampler;

        bool operator==(const BatchKey &rhs) const {
            return rhs.texture == texture &&
                   rhs.sampler == sampler;
        }

        bool operator!=(const BatchKey &rhs) const {
            return !(*this == rhs);
        }
    };

    struct BatchVertex2D {
        Vec2 uv;
        Color color;
        Vec3 position;
    };

private:
    [[nodiscard]]
    bool can_fit_quad() const;

    void set_batch_key(const BatchKey &key);

private:
    struct BatchState {
        PipelineHandle pipeline;
        BufferHandle vertex_buffer;
        BufferHandle index_buffer;

        struct DefaultSamplers {
            SamplerHandle nearest;
            SamplerHandle linear;
        } samplers;
    } m_state;

    std::vector<BatchVertex2D> m_vertices;
    std::vector<uint16_t> m_indices;

    std::optional<BatchKey> m_current_key;
    IRenderDevice *m_rd;
};
