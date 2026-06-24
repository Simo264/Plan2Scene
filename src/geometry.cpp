#include "geometry.hpp"
#include "types.hpp"

#include "dbscan.h"

#include <glm/ext/vector_double2.hpp>
#include <glm/geometric.hpp>
#include <glm/common.hpp>

#include <clipper2/clipper.h>

f64 calculate_signed_area(const std::vector<glm::dvec2>& contour)
{
  auto area = 0.0;
  auto n = contour.size();
  for (auto i = 0ul; i < n; ++i)
  {
    auto p1 = contour.at(i);
    auto p2 = contour.at((i + 1) % n);
    area += (p1.x * p2.y - p2.x * p1.y);
  }
  return area * 0.5;
}

BoundingBox2D calculate_bbox_2D(const std::vector<glm::dvec2>& polyline) 
{
  if (polyline.empty()) 
    return BoundingBox2D{ glm::dvec2{0.0, 0.0}, glm::dvec2{0.0, 0.0} };

  auto min_x = std::numeric_limits<f64>::max();
  auto min_y = std::numeric_limits<f64>::max();
  auto max_x = -std::numeric_limits<f64>::max();
  auto max_y = -std::numeric_limits<f64>::max();
  for (const auto& p : polyline) 
  {
    min_x = std::min(min_x, p.x);
    min_y = std::min(min_y, p.y);
    max_x = std::max(max_x, p.x);
    max_y = std::max(max_y, p.y);
  }

  return BoundingBox2D{
    .min = glm::dvec2{ min_x, min_y },
    .max = glm::dvec2{ max_x, max_y }
  };
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
    .min = glm::dvec2{ min_x, min_y },
    .max = glm::dvec2{ max_x, max_y }
  };
}

BoundingBox2D calculate_bbox_2D(const std::vector<glm::dvec2>& points, const std::vector<u32>& cluster_indices) 
{
  auto min_x = std::numeric_limits<f64>::max();
  auto min_y = std::numeric_limits<f64>::max();
  auto max_x = -std::numeric_limits<f64>::max();
  auto max_y = -std::numeric_limits<f64>::max();
  for (int idx : cluster_indices) 
  {
    const auto& p = points[idx];
    min_x = std::min(min_x, p.x);
    min_y = std::min(min_y, p.y);
    max_x = std::max(max_x, p.x);
    max_y = std::max(max_y, p.y);
  }

  return BoundingBox2D{
    .min = glm::dvec2{ min_x, min_y },
    .max = glm::dvec2{ max_x, max_y }
  };
}

std::array<Segment, 2> get_long_sides_bbox2d(const BoundingBox2D& bbox)
{
  auto dx = bbox.max.x - bbox.min.x;
  auto dy = bbox.max.y - bbox.min.y;
  if (dx > dy) 
  {
    // The X-axis is dominant: the long sides are horizontal
    Segment bottom = {
      .start = glm::dvec2(bbox.min.x, bbox.min.y),
      .end   = glm::dvec2(bbox.max.x, bbox.min.y),
      .layer = LayerType::NONE
    };
    Segment top = {
      .start = glm::dvec2(bbox.min.x, bbox.max.y),
      .end   = glm::dvec2(bbox.max.x, bbox.max.y),
      .layer = LayerType::NONE
    };
    return { bottom, top };
  } 
  else 
  {
    // The Y-axis is dominant: the long sides are vertical
    Segment left = {
      .start = glm::dvec2(bbox.min.x, bbox.min.y),
      .end   = glm::dvec2(bbox.min.x, bbox.max.y),
      .layer = LayerType::NONE
    };
    Segment right = {
      .start = glm::dvec2(bbox.max.x, bbox.min.y),
      .end   = glm::dvec2(bbox.max.x, bbox.max.y),
      .layer = LayerType::NONE
    };
    return { left, right };
  }
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

f64 detect_unit_scale(f64 area_bbox) 
{
  if (area_bbox > 10'000'000.0) return 0.001;   // mm
  if (area_bbox > 100'000.0)    return 0.01;    // cm
  if (area_bbox > 10'000.0)     return 0.0254;  // inches
  if (area_bbox > 1'000.0)      return 0.1;     // dm
  if (area_bbox > 100.0)        return 0.3048;  // feet
  return 1.0;                                   // m
}

void normalize_segments(f64 unit, std::vector<Segment>& segments)
{
  for (auto& seg : segments)
  {
    seg.start *= unit;
    seg.end *= unit;
  }
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

std::vector<glm::dvec2> sample_segments(const std::vector<Segment>& segments, i32 num_samples)
{
  auto points = std::vector<glm::dvec2>{};
  points.reserve(segments.size());
  for (const auto& s : segments)
  {
    for (auto i = 0; i <= num_samples; ++i) 
    {
      auto t = static_cast<f64>(i) / num_samples;
      auto x = s.start.x + t * (s.end.x - s.start.x);
      auto y = s.start.y + t * (s.end.y - s.start.y);
      points.push_back(glm::dvec2{x, y});
    }
  }
  return points;
}

std::vector<std::vector<u32>> calculate_clusters(std::vector<glm::dvec2>& sample_points,
                                                 f64 eps)
{
  auto dbscan = DBSCAN<glm::dvec2, f64>(); 
  constexpr auto min_points = 2;
  dbscan.Run(&sample_points, 2, eps, min_points);
  auto clusters = dbscan.Clusters;
  return clusters;
}

WallVertices get_wall_vertices(const glm::dvec2& gap_start,
                               const glm::dvec2& gap_end,
                               const SpatialHash& hash,
                               const std::vector<Edge>& edges,
                               const std::vector<glm::dvec2>& vertices) 
{
  auto result = WallVertices{};
  result.B = hash.find_nearest(gap_start);
  result.D = hash.find_nearest(gap_end);

  auto wall_dir = glm::normalize(vertices[result.B] - vertices[result.D]);

  auto nbrs_B = find_neighboors(result.B, edges);
  result.C = get_adjacent_vertex(wall_dir, result.B, nbrs_B, vertices);

  auto nbrs_D = find_neighboors(result.D, edges);
  result.E = get_adjacent_vertex(wall_dir, result.D, nbrs_D, vertices);
  return result;
}

bool are_parallel(const glm::dvec2& v1, 
                  const glm::dvec2& v2, 
                  f64 eps) 
{
  auto len1 = glm::length(v1);
  auto len2 = glm::length(v2);
  if (len1 < eps || len2 < eps) 
    return false;

  auto dot = glm::dot(v1, v2);
  auto cos_angle = dot / (len1 * len2);
  return std::abs(cos_angle) >= (1.0 - eps);
}

ProjResult project_point_on_segment(const glm::dvec2& p,
                                    const glm::dvec2& a,
                                    const glm::dvec2& b,
                                    f64 eps) 
{
  auto ab = b - a;
  auto ap = p - a;
  auto len_sq = glm::dot(ab, ab);
  if (len_sq < 1e-12) 
    return { a, false }; // a and b are the same point

  auto t = glm::dot(ap, ab) / len_sq;
  if (t > eps && t < (1.0 - eps)) 
    return {a + ab * t, true};
  return {a + ab * t, false};
}

VertexId split_edge(std::vector<glm::dvec2>& vertices,
                    std::vector<Edge>& edges,
                    VertexId v1,
                    VertexId v2,
                    const glm::dvec2& new_point,
                    LayerType layer) 
{
  // Add new vertex
  vertices.push_back(new_point);
  auto new_id = static_cast<VertexId>(vertices.size() - 1);

  // Remove the old edge
  std::erase_if(edges, [&](const Edge& e) { return (e.v1 == v1 && e.v2 == v2) || (e.v1 == v2 && e.v2 == v1); });

  // Create two new edges
  edges.push_back(Edge{v1, new_id, layer});
  edges.push_back(Edge{new_id, v2, layer});
  return new_id;
}

void close_wall_gap(glm::dvec2 gap_start,
                    glm::dvec2 gap_end,
                    LayerType type,
                    SpatialHash& hash,
                    std::vector<Edge>& edges) 
{
  auto& vertices = hash.vertices();

  // Find the four vertices B, C, D, E corresponding to the gap
  auto wall_vertices = get_wall_vertices(gap_start, gap_end, hash, edges, vertices);

  // Check if the edges B-D and C-E are already parallel
  auto vec_BD = vertices[wall_vertices.D] - vertices[wall_vertices.B];
  auto vec_CE = vertices[wall_vertices.E] - vertices[wall_vertices.C];
  if (are_parallel(vec_BD, vec_CE))
  {
    // if they are already parallel, we can just add the edges B-D and C-E
    edges.push_back(Edge{wall_vertices.B, wall_vertices.D, type});
    edges.push_back(Edge{wall_vertices.C, wall_vertices.E, type});
    return;
  }

  // We try to find a point on the opposite wall that is parallel to the gap. 
  // We can do this by projecting one of the vertices onto the opposite wall.
  auto proj_E = project_point_on_segment(vertices[wall_vertices.E],
                                          vertices[wall_vertices.B],
                                          vertices[wall_vertices.C]);
  if (proj_E.is_inside) 
  {
    // split the edge B-C and create E' (which will be the new vertex on B-C)      
    auto E_prime = split_edge(vertices, edges, wall_vertices.B, wall_vertices.C, proj_E.point);
    // the opposite side is now E' - E (which is parallel to B-D)
    edges.push_back(Edge{wall_vertices.B, wall_vertices.D, type});
    edges.push_back(Edge{E_prime, wall_vertices.E, type});
    return;
  }

  // Try projecting C onto the segment D-E
  auto proj_C = project_point_on_segment(vertices[wall_vertices.C],
                                          vertices[wall_vertices.D],
                                          vertices[wall_vertices.E]);

  if (proj_C.is_inside) 
  {
    auto C_prime = split_edge(vertices, edges, wall_vertices.D, wall_vertices.E, proj_C.point);
    // now we have B-D and C'-E (which are parallel)
    edges.push_back(Edge{wall_vertices.B, wall_vertices.D, type});
    edges.push_back(Edge{wall_vertices.C, C_prime, type});
    return;
  }

  throw std::runtime_error("Failed to close wall gap: could not find a parallel projection for either vertex.");
}

void doors_reconstruction(std::vector<Segment>& doors,
                          SpatialHash& hash,
                          std::vector<Edge>& edges)
{
  auto& vertices = hash.vertices();
  for(auto& door : doors) 
  { 
    close_wall_gap(door.start, door.end, LayerType::DOOR, hash, edges);
    door.start = vertices[hash.find_nearest(door.start)];
    door.end   = vertices[hash.find_nearest(door.end)];
  }
}

void windows_reconstruction(std::vector<glm::dvec2>& sample_points,
                            std::vector<std::vector<u32>> clusters,
                            SpatialHash& hash,
                            std::vector<Edge>& edges)
{
  for (auto i = 0ul; i < clusters.size(); ++i) 
  {
    const auto& cluster_indices = clusters[i];
    if (cluster_indices.empty()) 
      continue;
        
    auto box = calculate_bbox_2D(sample_points, cluster_indices);
    auto sides = get_long_sides_bbox2d(box);
    auto longest_side = sides.at(0);
    
    close_wall_gap(longest_side.start, longest_side.end, LayerType::WINDOW, hash, edges);
  }
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
