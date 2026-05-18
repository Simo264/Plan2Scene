#pragma once

#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_float3.hpp>

#include <vector>

#include "types.hpp"

enum class LayerType : i32
{ 
  NONE, WALL, WINDOW, DOOR 
};

struct Segment
{
  glm::dvec2 p1{ 0.0 }, p2{ 0.0 };
  LayerType layer { LayerType::NONE };
};

struct Polyline
{
  std::vector<glm::dvec2> vertices;
  bool closed{ false };
};

struct Vertex_PN
{
  glm::vec3 position{ 0.f };
  glm::vec3 normal{ 0.f };
};

struct BoundingBox
{
  glm::vec3 min{ 0.f };  // the bottom left corner
  glm::vec3 max{ 0.f };  // the top right corner
};

// Calculate the signed area of the contour using the shoelace formula.
auto calculate_signed_area(const Polyline& contour) -> f32;

// Calculates the bounding box for 2D points. The Z-coordinate is set to 0.0f by default.
auto calculate_bounding_box(const std::vector<glm::dvec2>& points) -> BoundingBox;
// Calculates the bounding box for 3D points
auto calculate_bounding_box(const std::vector<Vertex_PN>& vertices) -> BoundingBox;

// Calculate the unit scale based on the geometry
auto detect_unit_scale(const std::vector<glm::dvec2>& points) -> f32;

auto compute_polygon_offsetting(const std::vector<glm::dvec2>& inner_points, 
                                f32 thickness) -> std::vector<glm::dvec2>;