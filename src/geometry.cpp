#include "geometry.hpp"
#include "types.hpp"

#include <glm/ext/vector_double2.hpp>
#include <glm/geometric.hpp>
#include <glm/common.hpp>

#include <clipper2/clipper.h>
#include <print>

auto calculate_signed_area(const std::vector<glm::dvec2>& contour) -> f32
{
  auto area = 0.0;
  auto n = contour.size();
  for (size_t i = 0; i < n; ++i)
  {
    auto p1 = contour.at(i);
    auto p2 = contour.at((i + 1) % n);
    area += (p1.x * p2.y - p2.x * p1.y);
  }
  return area * 0.5;
}

BoundingBox2D calculate_bbox_2D(std::span<const Segment> segments)
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
  return BoundingBox2D{
    .min = glm::vec2{ f32(min_x), f32(min_y) },
    .max = glm::vec2{ f32(max_x), f32(max_y) }
  };
}

BoundingBox3D calculate_bbox_3D(const std::vector<Vertex_PN>& vertices) 
{
  auto min = vertices.front().position;
  auto max = min;
  for (const auto& p : vertices) 
  {
    min = glm::min(min, p.position);
    max = glm::max(max, p.position);
  }
  return BoundingBox3D{ min, max };
}

f32 detect_unit_scale(const std::vector<Segment>& wall_segments)
{
  auto bbox = calculate_bbox_2D(wall_segments);
  auto area = (bbox.max.x - bbox.min.x) * (bbox.max.y - bbox.min.y);

  std::println("Bounding box area: {}", area);

  if (area > 25'000'000.f)  return 0.001f;   // mm²
  if (area >   250'000.f)   return 0.01f;    // cm²
  if (area >     2'500.f)   return 0.1f;     // dm²
  return 1.0f;                                 // m²
}

void normalize_segments(f32 unit, std::span<Segment> segments)
{
  for (auto& seg : segments)
  {
    seg.p1 *= unit;
    seg.p2 *= unit;
  }
}


std::array<VertexId, 2> find_neighboors(VertexId vertex, const std::vector<GraphEdge>& edges)
{
  std::array<VertexId, 2> nbrs = { INVALID_VERTEX_ID, INVALID_VERTEX_ID };
  auto count = 0;
  for (const auto& e : edges)
  {
    if (e.v1 == vertex || e.v2 == vertex)
    {
      auto v = (e.v1 == vertex) ? e.v2 : e.v1;
      nbrs[count++] = v;

      if (count == 2) 
        break;
    }
  }
  return nbrs;
}

VertexId get_adjacent_vertex(const glm::dvec2& wall_dir, 
                             VertexId vertex_id,
                             const std::array<VertexId, 2>& vertex_neighbors, 
                             const std::vector<glm::dvec2>& vertices)
{
  VertexId vertex_prime_id = INVALID_VERTEX_ID;
  
  auto dir_1 = glm::normalize(vertices[vertex_neighbors[0]] - vertices[vertex_id]);
  auto dot_1 = std::abs(glm::dot(wall_dir, dir_1));
  auto dir_2 = glm::normalize(vertices[vertex_neighbors[1]] - vertices[vertex_id]);
  auto dot_2 = std::abs(glm::dot(wall_dir, dir_2));

  if(dot_1 < dot_2)
    vertex_prime_id = vertex_neighbors[0];
  else
    vertex_prime_id = vertex_neighbors[1];

  return vertex_prime_id;
}

// auto compute_polygon_offsetting(const std::vector<glm::dvec2>& inner_points, 
//                                 f32 thickness) -> std::vector<glm::dvec2>
// {
//   // Convert to Clipper2 format
//   auto innerPath = Clipper2Lib::PathD{};
//   for (const auto& p : inner_points) 
//     innerPath.push_back(Clipper2Lib::PointD(p.x, p.y));
// 
//   // Inflate the polygon outward
//   Clipper2Lib::PathsD solution = InflatePaths(
//       Clipper2Lib::PathsD{innerPath}, 
//       thickness, 
//       Clipper2Lib::JoinType::Miter, // Good for architectural corners
//       Clipper2Lib::EndType::Polygon // Closed polygon
//   );
// 
//   // Convert back to glm::dvec2
//   auto outer_points = std::vector<glm::dvec2>{};
//   if (!solution.empty() && !solution[0].empty()) 
//   {
//     for (const auto& pt : solution[0]) 
//     {
//       outer_points.emplace_back(pt.x, pt.y);
//     }
//   }
//   return outer_points;
// }

void build_triangulated_face(std::vector<Vertex_PN>& out_vertices,
                             std::vector<u32>& out_indices,
                             const std::vector<p2t::Triangle*> triangles,
                             f32 height,
                             const glm::vec3& normal)
{
  for (const auto& tri : triangles)
  {
    for (auto i = 0; i < 3; ++i) 
    {
      // poly2tri gives CCW winding in XY, so we emit points in order 0,1,2 which stays CCW
      auto p = tri->GetPoint(i);
      auto idx = static_cast<u32>(out_vertices.size());
      out_indices.push_back(idx);

      auto v = Vertex_PN{};
      v.position.x = static_cast<f32>(p->x);
      v.position.y = height;
      v.position.z = static_cast<f32>(p->y);
      v.normal = normal,
      out_vertices.push_back(v);
    }
  }
}

void extrude_walls(std::vector<Vertex_PN>& vertices, 
                   std::vector<u32>& out_indices,
                   f32 height,
                   const std::vector<glm::dvec2>& contour)
{
  constexpr glm::vec3 up(0.0f, 1.0f, 0.0f);
  
  for (auto i = 0u; i < contour.size(); ++i) 
  {
    auto p1 = contour[i];
    auto p2 = contour[(i + 1) % contour.size()];

    auto edge = glm::vec3(f32(p2.x - p1.x), 0.0f, f32(p2.y - p1.y));
    auto normal = glm::normalize(glm::cross(edge, up));

    auto BL = Vertex_PN{ .position={f32(p1.x), 0.f,    f32(p1.y)}, .normal=normal };
    auto BR = Vertex_PN{ .position={f32(p2.x), 0.f,    f32(p2.y)}, .normal=normal };
    auto TR = Vertex_PN{ .position={f32(p2.x), height, f32(p2.y)}, .normal=normal };
    auto TL = Vertex_PN{ .position={f32(p1.x), height, f32(p1.y)}, .normal=normal };

    auto base = static_cast<u32>(vertices.size());
    vertices.push_back(BL);
    vertices.push_back(BR);
    vertices.push_back(TR);
    vertices.push_back(TL);

    out_indices.push_back(base + 0);
    out_indices.push_back(base + 1);
    out_indices.push_back(base + 2);
    out_indices.push_back(base + 0);
    out_indices.push_back(base + 2);
    out_indices.push_back(base + 3);
  } 
}




