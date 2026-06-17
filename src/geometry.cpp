#include "geometry.hpp"
#include "types.hpp"

#include <glm/ext/vector_double2.hpp>
#include <glm/geometric.hpp>
#include <glm/common.hpp>

#include <clipper2/clipper.h>
#include <print>
#include <utility>

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

BoundingBox2D calculate_bbox_2D(const std::vector<Segment>& segments)
{
  auto min_x =  std::numeric_limits<f64>::max();
  auto min_y =  std::numeric_limits<f64>::max();
  auto max_x = -std::numeric_limits<f64>::max();
  auto max_y = -std::numeric_limits<f64>::max();
  for (const auto& seg : segments)
  {
    min_x = std::min({min_x, seg.start.x, seg.end.x});
    min_y = std::min({min_y, seg.start.y, seg.end.y});
    max_x = std::max({max_x, seg.start.x, seg.end.x});
    max_y = std::max({max_y, seg.start.y, seg.end.y});
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

f32 detect_unit_scale(f32 area_bbox)
{
  if (area_bbox > 25'000'000.f)  return 0.001f;   // mm^2
  if (area_bbox >   250'000.f)   return 0.01f;    // cm^2
  if (area_bbox >     2'500.f)   return 0.1f;     // dm^2
  return 1.0f;                                    // m^2
}

void normalize_segments(f32 unit, std::vector<Segment>& segments)
{
  for (auto& seg : segments)
  {
    seg.start *= unit;
    seg.end *= unit;
  }
}

std::vector<Edge> vertex_snapping(const std::vector<Segment> walls_segments, SpatialHash& hash)
{
  auto edges = std::vector<Edge>{};

  auto wall_segments_view = std::array{ walls_segments };
  for (const auto& seg : wall_segments_view | std::views::join)
  {
    auto v1 = hash.snap(seg.start);
    auto v2 = hash.snap(seg.end);
    if (v1 != v2)
      edges.push_back(Edge{ v1, v2, seg.layer });
  }
  return edges;
}

std::array<VertexId, 2> find_neighboors(VertexId vertex, const std::vector<Edge>& edges)
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

ProjResult project_onto_segment(const glm::dvec2& p, const glm::dvec2& v1, const glm::dvec2& v2) 
{
  auto v = v2 - v1;
  auto w = p - v1;
  double len_sq = glm::dot(v, v);
  if (len_sq < 1e-6) 
    return ProjResult{ v1, false };
  
  double t = glm::dot(w, v) / len_sq;
  if (t > 1e-4 && t < (1.0 - 1e-4)) 
      return { v1 + v * t, true };
  return { v1, false };
}




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


void extrude_face(std::vector<Vertex_PN>& vertices, 
                  std::vector<u32>& out_indices,
                  f32 base_height,
                  f32 top_height,
                  const Face& face)
{
  constexpr glm::vec3 up(0.0f, 1.0f, 0.0f);
  const auto& contour = face.vertices;
  
  for (auto i = 0u; i < contour.size(); ++i) 
  {
    auto p1 = contour[i];
    auto p2 = contour[(i + 1) % contour.size()];
    auto edge = glm::vec3(f32(p2.x - p1.x), 0.0f, f32(p2.y - p1.y));
    auto normal = glm::normalize(glm::cross(edge, up));

    auto BL = Vertex_PN{ .position={f32(p1.x), base_height, f32(p1.y)}, .normal=normal };
    auto BR = Vertex_PN{ .position={f32(p2.x), base_height, f32(p2.y)}, .normal=normal };
    auto TR = Vertex_PN{ .position={f32(p2.x), top_height, f32(p2.y)},  .normal=normal };
    auto TL = Vertex_PN{ .position={f32(p1.x), top_height, f32(p1.y)},  .normal=normal };

    auto base = static_cast<u32>(vertices.size());
    vertices.push_back(BL); vertices.push_back(BR);
    vertices.push_back(TR); vertices.push_back(TL);

    out_indices.push_back(base + 0); out_indices.push_back(base + 1); out_indices.push_back(base + 2);
    out_indices.push_back(base + 0); out_indices.push_back(base + 2); out_indices.push_back(base + 3);
  } 
}

void center_mesh(std::vector<Vertex_PN>& vertices)
{
  auto bbox = calculate_bbox_3D(vertices);
  auto center = (bbox.min + bbox.max) * 0.5f;
  for (auto& v : vertices) 
  {
    v.position.x -= center.x;
    v.position.y -= center.y;
    v.position.z -= center.z;
  }
}
