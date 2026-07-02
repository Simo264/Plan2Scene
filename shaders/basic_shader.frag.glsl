#version 460 core

layout(location = 0) in vec3 vs_out_normal_world_space;
layout(location = 1) in vec2 vs_out_text_coord;
layout(location = 2) in vec3 vs_out_frag_world_space;

layout(location = 0) out vec4 fs_out_color;

layout(binding = 0) uniform sampler2D u_texture_color;

void main()
{
  vec3 surface_color = texture(u_texture_color, vs_out_text_coord).rgb;
  vec3 result = surface_color;
  fs_out_color = vec4(result, 1.0);
}