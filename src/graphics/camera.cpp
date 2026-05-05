#include "camera.hpp"

#include <glm/ext/vector_float4.hpp>

glm::mat4 Camera::canonical_to_camera() const
{
	// The camera transformation, that converts points from the canonical coordinate system to camera 
	// coordinates system.
	// Let's denote the eye position e, the gaze direction g and the view-up vector t. 
	// These vectors allow us to construct a coordinate system with origin e and the basis:
	// - w = - (g/||g||)
	// - u = (t x w) / (|| t x w ||)
	// - v = w x u
	// 
	// We just need to convert these coordinates into into the camera frame coordinate system. 
	// We can do this using the following matrix transformation:
	// 
	//                          | x_u   y_u   z_u   -x_e |
	//         | u v w e |^1    | x_v   y_v   z_v   -y_e |
	// M_cam = | 0 0 0 1 |    = | x_w   y_w   z_w   -z_e |
	//                          | 0     0     0      1   
	
	// auto w = -glm::normalize(gaze);
	// auto u = glm::normalize(glm::cross(up, w));
	// auto v = glm::cross(w, u);
	// auto m_cam = glm::mat4{
	//   glm::vec4(u.x, u.y, u.z, 0.0f),
	//   glm::vec4(v.x, v.y, v.z, 0.0f),
	//   glm::vec4(w.x, w.y, w.z, 0.0f),
	//   glm::vec4(-glm::dot(u, eye), -glm::dot(v, eye), -glm::dot(w, eye), 1.0f)
	// };
	// return m_cam;
	
	auto rot = glm::mat4_cast(glm::conjugate(orientation));
  auto tra = glm::translate(glm::mat4(1.0f), -eye);
  return rot * tra;
}

glm::mat4 Camera::get_perspective() const
{ 
	// In perspective, the further away an object is (z larger), the smaller it should appear on the screen. 
	// Mathematically, this means that the coordinates x and y must be divided by z (proportionally 1/z).
	//
	// To implement perspective in 3D we use the convention of the camera at the origin looking towards -z. 
	// We define two planes: the near plane (n) and the far plane (f) which are the same as seen in orthographic projection. 
	// The resulting matrix is this:
	//
	//     | n		0 	0		0 	|
	//     | 0  	n  	0 	0 	|
	// P = | 0  	0  	n+f -fn |
	//     | 0  	0   1   0   |
	// 
	// The perspective matrix simply maps the perspective view volume (or frustum) to the orthographic view volume 
	// which is axis-aligned box, and then we can use the orthographic transform to get the canonical view volume. 
	// Concatenating P with M_ortho we get the perspective projection matrix:
	// 
	//   					            | (2n)/(r-l)   	0         		(l+r)/(l-r) 	0 					|
	//                        | 0         		(2n)/(t-b)   	(b+t)/(b-t) 	0						|
	// M_per = M_ortho * P  = | 0         		0         		(f+n)/(n-f)		(2n)/(f-n)	|
	//                        | 0         		0         		1         		0           |

	auto t = near * glm::tan(fovy / 2.0f);
	auto r = t * aspect;
	auto b = -t;
	auto l = -r;	
  auto m_per = glm::mat4(
    glm::vec4(2.0f*near/(r-l),  0.0f,             0.0f,                         0.0f),
    glm::vec4(0.0f,             2.0f*near/(t-b),  0.0f,                         0.0f),
    glm::vec4((r+l)/(r-l),      (t+b)/(t-b),     -(far+near)/(far-near),       -1.0f),
    glm::vec4(0.0f,             0.0f,            -(2.0f*far*near)/(far-near),   0.0f)
  );
	return m_per;
}

glm::mat4 Camera::get_orthographic() const
{
	// The projection transformation, that moves points from camera space to the view volume.
	// In the case of orthographic view volume, this volume is an axis-aligned box with where its 
	// side are [l,r] x [b,t] x [f,n].
	// We need to perform another transform from orthographic to canonical view volume and this 
	// is windowing transform, we can simply substitute the bounds of the orthographic and the 
	// canonical view volume to obtain the following matrix: 
	// 
	//            | 2/(r-l)   0         0         -(r+l)/(r-l) |
	//            | 0         2/(t-b)   0         -(t+b)/(t-b) |
	// M_ortho =  | 0         0         2/(n-f)   -(n+f)/(n-f) |
	//            | 0         0         0         1            |
	
	auto t = near * glm::tan(fovy / 2.0f);
	auto r = t * aspect;
	auto b = -t;
	auto l = -r;	
	auto m_ortho = glm::mat4(
  	glm::vec4(2.0f/(r-l),   0.0f,          	0.0f,        						0.0f),
  	glm::vec4(0.0f,        	2.0f/(t-b),			0.0f,        						0.0f),
  	glm::vec4(0.0f,         0.0f,     		 -2.0f/(far-near),       	0.0f),
  	glm::vec4(-(r+l)/(r-l), -(t+b)/(t-b),  -(far+near)/(far-near), 	1.0f)
	);
	return m_ortho;
}

void Camera::rotate_yaw(f32 angle_radians)
{
  glm::quat q = glm::angleAxis(angle_radians, glm::vec3(0, 1, 0));
  orientation = glm::normalize(q * orientation);
}

void Camera::rotate_pitch(f32 angle_radians)
{
  glm::quat q = glm::angleAxis(angle_radians, glm::vec3(1, 0, 0));
  orientation = glm::normalize(orientation * q);
}

void Camera::rotate_roll(f32 angle_radians)
{
  glm::quat q = glm::angleAxis(angle_radians, glm::vec3(0, 0, 1));
  orientation = glm::normalize(orientation * q);
}