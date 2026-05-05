#pragma once

#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_float3.hpp>

#include <vector>

#include "types.hpp"

struct Segment
{
  glm::dvec2 p1, p2;
};

struct Polyline
{
  std::vector<glm::dvec2> points;
  bool closed{ false };
};

// struct Vertex3d
// {
//   glm::dvec3 position;
//   glm::dvec3 normal;
// };

struct Vertex
{
  glm::vec3 position;
  glm::vec3 normal;
};

// Calculate the Euclidean distance between two 2D points
auto distance(glm::dvec2 p1, glm::dvec2 p2) -> f64;
// Calculate the signed area of the contour using the shoelace formula.
auto signed_area(const Polyline& contour) -> f64;
