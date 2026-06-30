#pragma once

#include <glm/ext/vector_float3.hpp>
#include <glm/ext/matrix_float4x4.hpp>

struct Transformation
{	
	glm::vec3 position{ 0.f };
	glm::vec3 scale{ 1.0f };
	glm::vec3 rotation{ 0.f }; // the euler rotation in radians

	// Calculate the transformation matrix `M` which leads vertices from local coordinate system to world coordinate system.
	// This matrix `M` directly implements the transformation of a vertex from local space to world space.	
	glm::mat4 M{ 1.0f };
	void update_transformation();
};