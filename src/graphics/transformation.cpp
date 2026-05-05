#include "transformation.hpp"

#include <glm/trigonometric.hpp>

void Transformation::update_tranformation()
{
 	// The explicit form of the affine tranfsormation:
 	//
 	//  	M = | u  v  w  e |
 	//	
 	// Where:
 	// 	- the vectors `(u,v,w)` represent the orthonormal basis of the local coordinate system
 	// 	- the point `e` represents the translation in world space.
	
  auto S = glm::mat3 {
   	scale.x, 0.0f, 0.0f, 
   	0.0f, scale.y, 0.0f, 
   	0.0f, 0.0f, scale.z 
  };
  
  auto cx = glm::cos(rotation.x);
  auto sx = glm::sin(rotation.x);
  auto R_x = glm::mat3 {
      1.0f,  0.0f, 0.0f,
      0.0f,  cx,   sx,
      0.0f, -sx,   cx
  };
  auto cy = glm::cos(rotation.y);
  auto sy = glm::sin(rotation.y);
  auto R_y = glm::mat3 {
       cy,  0.0f, sy,
      0.0f, 1.0f, 0.0f,
      -sy,  0.0f, cy
  };
  
  auto cz = glm::cos(rotation.z);
  auto sz = glm::sin(rotation.z);
  auto R_z = glm::mat3 {
       cz,  sz,   0.0f,
      -sz,  cz,   0.0f,
      0.0f, 0.0f, 1.0f 
  };
  
  auto R = R_z * R_y * R_x;
  auto RS = R * S;
  M = glm::mat4 {
    glm::vec4(RS[0], 0.0f),
    glm::vec4(RS[1], 0.0f),
    glm::vec4(RS[2], 0.0f),
    glm::vec4(position, 1.0f)    
  }; 
}