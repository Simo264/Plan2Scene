#version 460 core

in vec3 vs_out_normal_world_space;
out vec4 fs_out_color;

void main()
{
  vec3 surface_color;
  
  // surface_color = vec3(0.3, 0.8, 0.15, 1);
  // 
  vec3 n = normalize(vs_out_normal_world_space);
  // map from [-1, 1] to [0, 1]
  n = n * 0.5 + 0.5;
  surface_color = n;
  
  fs_out_color = vec4(surface_color, 1.0);
}