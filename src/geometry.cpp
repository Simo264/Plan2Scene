#include "geometry.hpp"

#include <cmath>

auto distance(Vec2 p1, Vec2 p2) -> double { return std::hypot(p1.x - p2.x, p1.y - p2.y); }

auto signed_area(const Polyline& contour) -> double
{
  const auto& points = contour.points;

  // Compute the signed area of the contour using the shoelace formula.
  auto area = double{ 0.f };
  for(auto i = 0u; i < points.size(); ++i)
  {
    auto p1 = points.at(i);
    auto p2 = points.at((i + 1) % points.size());
    area += (p1.x * p2.y - p2.x * p1.y);
  }
  return area * 0.5f;
}
