#pragma once

inline const char *g_vertex_shader_source = R"(
#version 330 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec2 a_uv;

out vec2 v_uv;

uniform mat4 u_view_proj;
uniform mat4 u_model;

void main() {
    v_uv = a_uv;
    gl_Position = u_view_proj * u_model * vec4(a_pos, 1.0);
}
)";

inline const char *g_fragment_shader_source = R"(
#version 330 core

in vec2 v_uv;

out vec4 color;

uniform sampler2D u_texture;
uniform vec4 u_color;

void main() {
    color = u_color * texture(u_texture, v_uv);
}
)";
