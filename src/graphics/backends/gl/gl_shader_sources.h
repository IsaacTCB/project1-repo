#pragma once

inline const char *const g_vertex_shader_source = R"(
#version 330 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in uint a_packed_color;

out vec2 v_uv;
out vec4 v_color;

uniform mat4 u_view_proj;

vec4 unpack_rgba8888(uint packed_color) {
    return vec4(
        float((packed_color >>  0) & 0xFFu),
        float((packed_color >>  8) & 0xFFu),
        float((packed_color >> 16) & 0xFFu),
        float((packed_color >> 24) & 0xFFu)
    ) / 255.0;
}

void main() {
    v_uv = a_uv;
    v_color = unpack_rgba8888(a_packed_color);
    gl_Position = u_view_proj * vec4(a_pos, 1.0);
}

)";

inline const char *const g_fragment_shader_source = R"(
#version 330 core

in vec2 v_uv;
in vec4 v_color;

out vec4 color;

uniform sampler2D u_texture;

void main() {
    color = v_color * texture(u_texture, v_uv);
}

)";
