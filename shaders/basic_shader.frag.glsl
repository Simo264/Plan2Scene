#version 460 core

layout(location = 0) in vec3 vs_out_normal_world_space;
layout(location = 1) in vec2 vs_out_text_coord;
layout(location = 2) in vec3 vs_out_frag_world_space;

layout(location = 0) out vec4 fs_out_color;

layout(location = 0) uniform vec3 u_light_pos;
layout(location = 1) uniform float u_light_power;

layout(binding = 0) uniform sampler2D u_texture_color;

const float PI = 3.14159265359;

void main()
{
  vec3 surface_color = texture(u_texture_color, vs_out_text_coord).rgb;

  vec3 N = normalize(vs_out_normal_world_space);
  vec3 L = u_light_pos - vs_out_frag_world_space;
  float distance = length(L);
  L = normalize(L);

  float irradiance = u_light_power / (4.0 * PI * distance * distance);
  float NdotL = max(dot(N, L) , 0.0);
  vec3 diffuse = (surface_color / PI) * irradiance * NdotL;
  vec3 ambient = 0.05 * surface_color; 
  vec3 result = ambient + diffuse;
  fs_out_color = vec4(result, 1.0);
}