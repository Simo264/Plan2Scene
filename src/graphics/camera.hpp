#pragma once

#include "../types.hpp"

#include <glm/ext/vector_float3.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/quaternion.hpp>

class Camera
{
public:
  Camera() = default;
  Camera(f32 n, f32 f, f32 fovy, f32 aspect) : 
  	eye{0.0f, 1.0f, 5.0f},
  	orientation{glm::quat(1.0f, 0.0f, 0.0f, 0.0f)},
  	near{n},
  	far{f},
  	fovy{glm::radians(fovy)},
  	aspect{aspect}
  {}

	// Converts points from the canonical coordinate system to camera coordinates system.
	glm::mat4 canonical_to_camera() const;
	// Returns the perspective projection matrix 
	glm::mat4 get_perspective() const;
	// Returns the orthographics projection matrix
	glm::mat4 get_orthographic() const;
	
	void rotate_yaw(f32 angle_radians);
  void rotate_pitch(f32 angle_radians);
  void rotate_roll(f32 angle_radians);
 
  auto gaze() const { return orientation * glm::vec3(0, 0, -1); }
  auto up() const { return orientation * glm::vec3(0, 1,  0); }
  auto right() const { return orientation * glm::vec3(1, 0,  0); }
  
  // The `euler_angles` parameter must be in radians 
  void set_orientation(const glm::vec3& euler_angles) { orientation = glm::quat(euler_angles); }
  // Returns the angles in radians
  auto get_euler_angles() const { return glm::eulerAngles(orientation); }
  
	glm::vec3 eye; 	// camera position
	glm::quat orientation;

	f32 near;				// near plane distance
	f32 far;				// far plane distance
	f32 fovy; 			// vertical fov in radians
	f32 aspect;			// the aspect ratio of the viewport
};