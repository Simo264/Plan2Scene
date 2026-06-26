#version 460 core

layout(location = 0) in vec3 vs_out_normal_world_space;
layout(location = 1) in vec3 vs_out_frag_world_space;

layout(location = 0) out vec4 fs_out_color;

void main()
{
  vec3 n = normalize(vs_out_normal_world_space);
  n = n * 0.5 + 0.5;
  fs_out_color = vec4(n, 1.0);
}