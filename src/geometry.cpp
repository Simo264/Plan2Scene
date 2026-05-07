#include "geometry.hpp"

#include <glm/ext/vector_double2.hpp>
#include <glm/geometric.hpp>
#include <glm/common.hpp>

auto calculate_signed_area(const Polyline& contour) -> f32
{
  const auto& points = contour.points;
  
  auto area =  0.f;
  for(auto i = 0u; i < points.size(); ++i)
  {
    auto p1 = points.at(i);
    auto p2 = points.at((i + 1) % points.size());
    area += (p1.x * p2.y - p2.x * p1.y);
  }
  return area * 0.5f;
}

auto calculate_bounding_box(const std::vector<glm::dvec2>& points) -> BoundingBox 
{
  if (points.empty())
    return BoundingBox{};
  
  auto min = glm::vec3{ std::numeric_limits<float>::max() };
  auto max = glm::vec3{ std::numeric_limits<float>::lowest() };
  for (const auto& p : points) 
  {
    auto px = static_cast<f32>(p.x);
    auto py = static_cast<f32>(p.y);
    min.x = glm::min(min.x, px);
    min.y = glm::min(min.y, py);
    max.x = glm::max(max.x, px);
    max.y = glm::max(max.y, py);
  }
  min.z = 0.f;
  max.z = 0.f;
  return BoundingBox{ min, max };
}

auto calculate_bounding_box(const std::vector<Vertex>& vertices) -> BoundingBox 
{
  if (vertices.empty())
    return BoundingBox{};

  auto min = vertices.front().position;
  auto max = min;
  for (const auto& p : vertices) 
  {
    min = glm::min(min, p.position);
    max = glm::max(max, p.position);
  }

  return BoundingBox{ min, max };
}

auto detect_unit_scale(const std::vector<glm::dvec2>& points) -> f32 
{
  if (points.empty()) 
    return 1.0f;

  auto bbox = calculate_bounding_box(points);

  // Calculate the diagonal distance of the floor plan (the "extent")
  auto extent = glm::distance(bbox.min, bbox.max);

  // Heuristic unit detection:
  // If extent > 5000, it's likely millimeters (e.g., 5000mm = 5m) -> scale 0.001
  // If extent > 500, it's likely centimeters (e.g., 500cm = 5m)   -> scale 0.01
  // If extent > 50, it's likely decimeters (e.g., 50dm = 5m)      -> scale 0.1
  if (extent > 5000.0f)
    return 0.001f;
  else if (extent > 500.0f)
    return 0.01f;
  else if (extent > 50.0f) 
    return 0.1f;

  return 1.0f; // Already in meters
}
