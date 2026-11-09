#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

struct VertexAttribute {
    enum class Semantic : uint8_t {
        POSITION,
        TEXCOORD,
        COLOR,
        NORMAL,

        UNKNOWN,
    } semantic = Semantic::UNKNOWN;

    enum class Format : uint8_t {
        /* signed ints */
        INT8,
        INT16,
        INT32,

        /* unsigned ints */
        UINT8,
        UINT16,
        UINT32,

        /* floats */
        FLOAT,
        FLOAT2,
        FLOAT3,
        FLOAT4,

        /* colors */
        RGB5551,
        RGB565,
        RGBA4444,
        RGBA8888,

    } format;

    size_t offset;
};

struct VertexLayout {
    std::span<const VertexAttribute> attributes;
    size_t stride;
};
