#version 460 core

layout(location = 0) in vec3 vs_out_normal_world_space;
layout(location = 1) in vec3 vs_out_frag_world_space;

layout(location = 0) out vec4 fs_out_color;

void main()
{
  vec3 surface_color;
  
  // surface_color = vec3(0.3, 0.8, 0.15, 1);
  
  vec3 n = normalize(vs_out_normal_world_space);
  n = n * 0.5 + 0.5;
  surface_color = n;
  fs_out_color = vec4(surface_color, 1.0);
}