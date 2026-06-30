#version 460 core

layout(location = 0) in vec3 vs_out_normal_world_space;
layout(location = 1) in vec2 vs_out_text_coord;
layout(location = 2) in vec3 vs_out_frag_world_space;

layout(location = 0) out vec4 fs_out_color;

layout(location = 0) uniform vec3 u_view_pos;
layout(location = 1) uniform vec3 u_light_pos;

layout(binding = 2) uniform sampler2D u_texture_color;

// const vec3 BASE_COLOR = vec3(0.75, 0.75, 0.75);
const vec3 LIGHT_COLOR = vec3(1.0, 1.0, 1.0);
const float AMBIENT_STRENGTH = 0.15;

void main()
{
  vec3 surface_color = texture(u_texture_color, vs_out_text_coord).rgb;
  
  vec3 n = normalize(vs_out_normal_world_space);
  vec3 light_dir = normalize(u_light_pos - vs_out_frag_world_space);
  
  float diff = max(dot(n, light_dir), 0.0);
  
  float dist = length(u_light_pos - vs_out_frag_world_space);
  float attenuation = 1.0 / (1.0 + 0.045 * dist + 0.0075 * dist * dist);
  
  vec3 ambient = AMBIENT_STRENGTH * surface_color;
  vec3 diffuse = diff * attenuation * LIGHT_COLOR * surface_color;
  
  vec3 result = ambient + diffuse;
  fs_out_color = vec4(result, 1.0);
}