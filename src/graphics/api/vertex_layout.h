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

        /* floats */
        FLOAT,
        FLOAT2,
        FLOAT3,
        FLOAT4,

    } format;

    size_t offset;
};

struct VertexLayout {
    std::span<const VertexAttribute> attributes;
    size_t stride;
};
