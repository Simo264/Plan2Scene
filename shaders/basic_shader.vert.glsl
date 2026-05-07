#version 460 core

layout(location = 0) in vec3 in_pos; 			// in local coordinate system
layout(location = 1) in vec3 in_normal;

layout(location = 0) uniform mat4 mat_transform;
layout(location = 1) uniform mat4 mat_cam; 
layout(location = 2) uniform mat4 mat_per;

layout(location = 0) out vec3 vs_out_normal_world_space;
 
void main()
{
  vec4 p_local_space = vec4(in_pos, 1.0f);
  // frame to canonical space
  vec4 p_world_space = mat_transform * p_local_space;
  // canonical to camera space
  vec4 p_camera_space = mat_cam * p_world_space;
  // camera to clip space (camera to perspective)
  vec4 p_clip_space = mat_per * p_camera_space;
  
  // normal transformation for non-uniform scaling: N = (M^-1)^T
  mat3 normal_matrix = transpose(inverse(mat3(mat_transform)));
  vec3 normal_world_space = normal_matrix * in_normal;
  gl_Position = p_clip_space;
  vs_out_normal_world_space = normal_world_space;
}