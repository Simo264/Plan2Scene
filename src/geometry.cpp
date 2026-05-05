#include "geometry.hpp"

#include <glm/ext/vector_double2.hpp>
#include <cmath>

auto distance(glm::dvec2 p1, glm::dvec2 p2) -> f64 { return std::hypot(p1.x - p2.x, p1.y - p2.y); }

auto signed_area(const Polyline& contour) -> f64
{
  const auto& points = contour.points;
  
  auto area = f64{ 0.f };
  for(auto i = 0u; i < points.size(); ++i)
  {
    auto p1 = points.at(i);
    auto p2 = points.at((i + 1) % points.size());
    area += (p1.x * p2.y - p2.x * p1.y);
  }
  return area * 0.5f;
}
