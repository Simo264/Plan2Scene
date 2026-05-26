#pragma once

#include "types.hpp"

#include <poly2tri/sweep/cdt.h>
#include <vector>
#include <span>
#include <ranges>
#include <concepts>
#include <limits>

// Calculate the signed area of the contour using the shoelace formula.
auto calculate_signed_area(const std::vector<glm::dvec2>& contour) -> f32;

// Calculates the bounding box for 3D points
auto calculate_bounding_box_3D(const std::vector<Vertex_PN>& vertices) -> BoundingBox;

// Calculate the unit scale based on the geometry — accepts any range of Segment
template<std::ranges::input_range R>
requires std::same_as<std::ranges::range_value_t<R>, Segment>
static auto detect_unit_scale(R&& segments) -> f32
{  
  auto min_x =  std::numeric_limits<f64>::max();
  auto min_y =  std::numeric_limits<f64>::max();
  auto max_x = -std::numeric_limits<f64>::max();
  auto max_y = -std::numeric_limits<f64>::max();

  for (const auto& seg : segments) 
  {
    min_x = std::min({min_x, seg.p1.x, seg.p2.x});
    min_y = std::min({min_y, seg.p1.y, seg.p2.y});
    max_x = std::max({max_x, seg.p1.x, seg.p2.x});
    max_y = std::max({max_y, seg.p1.y, seg.p2.y});
  }

  auto extent = std::hypot(max_x - min_x, max_y - min_y);

  if (extent > 5000.0)  return 0.001f;
  if (extent > 500.0)   return 0.01f;
  if (extent > 50.0)    return 0.1f;
  return 1.0f;
}

void normalize_segments(f32 unit, std::span<Segment> segments);


auto compute_polygon_offsetting(const std::vector<glm::dvec2>& inner_points, 
                                f32 thickness) -> std::vector<glm::dvec2>;

void build_floor(std::vector<Vertex_PN>& out_vertices,
                 std::vector<u32>& out_indices,
                 const std::vector<p2t::Triangle*> floor_triangles);

void build_ceil(std::vector<Vertex_PN>& out_vertices, 
                std::vector<u32>& out_indices,
                f32 H,
                const std::vector<p2t::Triangle*> triangle_list);


void extrude_walls(std::vector<Vertex_PN>& vertices, 
                   std::vector<u32>& out_indices,
                   f32 H,
                   const std::vector<glm::dvec2>& inner_points,
                   const std::vector<glm::dvec2>& outer_points);
                
void build_wall_top_cap(std::vector<Vertex_PN>& out_vertices,
                        std::vector<u32>& out_indices,
                        f32 H,
                        const std::vector<glm::dvec2>& inner_points,
                        const std::vector<glm::dvec2>& outer_points);

