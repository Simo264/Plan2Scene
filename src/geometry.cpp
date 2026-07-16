#include "geometry.hpp"
#include "types.hpp"
#include "globals.hpp"

#include "dbscan.h"

#include <poly2tri/sweep/cdt.h>

#include <glm/geometric.hpp>
#include <glm/common.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

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


void normalize_segments(f64 unit, std::vector<Segment>& segments)
{
  for (auto& seg : segments)
  {
    seg.start *= unit;
    seg.end *= unit;
  }
}

void center_mesh(std::vector<Segment>& walls, std::vector<Segment>& doors, std::vector<Segment>& windows)
{
  auto bbox = BoundingBox2D(walls);
  auto center = (bbox.min + bbox.max) * 0.5;
  auto translate = [&center](Segment& s) 
  {
    s.start -= center;
    s.end   -= center;
  };

  for (auto& s : walls)   translate(s);
  for (auto& s : doors)   translate(s);
  for (auto& s : windows) translate(s);
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

VertexId get_adjacent_vertex(glm::dvec2 wall_dir, 
                             VertexId vertex_id,
                             std::array<VertexId, 2> vertex_neighbors, 
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

std::vector<std::vector<u32>> calculate_clusters(std::vector<glm::dvec2>& sample_points, f64 eps)
{
  auto dbscan = DBSCAN<glm::dvec2, f64>(); 
  constexpr auto min_points = 2;
  dbscan.Run(&sample_points, 2, eps, min_points);
  auto clusters = dbscan.Clusters;
  return clusters;
}

WallVertices get_wall_vertices(glm::dvec2 gap_start,
                               glm::dvec2 gap_end,
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

bool are_vectors_parallel(glm::dvec2 v1, glm::dvec2 v2, f64 eps)
{
  auto len1 = glm::length(v1);
  auto len2 = glm::length(v2);
  if (len1 < eps || len2 < eps) 
    return false;

  auto dot = glm::dot(v1, v2);
  auto cos_angle = dot / (len1 * len2);
  return std::abs(cos_angle) >= (1.0 - eps);
}

void close_wall_gap(glm::dvec2 gap_start,
                    glm::dvec2 gap_end,
                    SegmentLayer type,
                    SpatialHash& hash,
                    std::vector<Edge>& edges,
                    f64 width_scale)
{
  auto& vertices = hash.vertices();
  auto wall_vertices = get_wall_vertices(gap_start, gap_end, hash, edges, vertices);
  auto B = vertices[wall_vertices.B];
  auto C = vertices[wall_vertices.C];
  auto D = vertices[wall_vertices.D];
  auto E = vertices[wall_vertices.E];

  auto vec_BD = D - B;
  auto vec_CE = E - C;
  if (!are_vectors_parallel(vec_BD, vec_CE))
  {
    g_logger.push_message(LogMessage{
      "Warning: opening jambs not parallel. Fix this opening in the source DXF for correct results.",
      LogLevel::Warning
    });
  }

  auto mid_BD = (B + D) * 0.5;
  auto mid_CE = (C + E) * 0.5;
  vertices[wall_vertices.B] = mid_BD + (B - mid_BD) * width_scale;
  vertices[wall_vertices.D] = mid_BD + (D - mid_BD) * width_scale;
  vertices[wall_vertices.C] = mid_CE + (C - mid_CE) * width_scale;
  vertices[wall_vertices.E] = mid_CE + (E - mid_CE) * width_scale;
  edges.push_back(Edge{wall_vertices.B, wall_vertices.D, type});
  edges.push_back(Edge{wall_vertices.C, wall_vertices.E, type});
}

void doors_reconstruction(std::vector<Segment>& doors,
                          SpatialHash& hash,
                          std::vector<Edge>& edges)
{
  auto& vertices = hash.vertices();
  for (auto i = 0ul; i < doors.size(); ++i)
  { 
    auto& door = doors[i];
    try
    {
      auto target_width = g_config.door_width;
      
      auto current_width = glm::distance(door.start, door.end);
      auto width_scale = target_width / current_width;           
      close_wall_gap(door.start, door.end, SegmentLayer::Door, hash, edges, width_scale);
      door.start = vertices[hash.find_nearest(door.start)];
      door.end   = vertices[hash.find_nearest(door.end)];
    } 
    catch (const std::exception& e) 
    {
      throw std::runtime_error(std::format("Door reconstruction failed at door index {} (of {}):\n{}",i, doors.size(), e.what()));
    }
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
        
    auto box = BoundingBox2D(sample_points, cluster_indices);
    auto sides = box.get_long_sides();
    auto longest_side = sides.at(0);
    try 
    {
      auto target_width = g_config.window_width;
      auto current_width = glm::distance(longest_side.start, longest_side.end);
      auto width_scale = target_width / current_width;              
      close_wall_gap(longest_side.start, longest_side.end, SegmentLayer::Window, hash, edges, width_scale);
    } 
    catch (const std::exception& e)
    {
      throw std::runtime_error(std::format("Window reconstruction failed at cluster index {} (of {}):\n{}",i, clusters.size(), e.what()));
    }
  }
}

OpeningInstance compute_opening_instance(const Face& face,
                                         OpeningType type,
                                         f32 z_min,
                                         f32 z_max)
{
  auto op = OpeningInstance{};
  op.type = type;

  auto center_2d = face.calculate_center();
  op.height = z_max - z_min;
  op.center = glm::vec3(center_2d.x, z_min + op.height * 0.5, center_2d.y);

  // Find longest side of the face
  auto max_edge_idx = 0;
  auto max_len_sqr = 0.0;
  for (auto i = 0u; i < face.vertices.size(); ++i) 
  {
    auto v1 = face.vertices[i];
    auto v2 = face.vertices[(i + 1) % face.vertices.size()];
    auto len_sqr = glm::length2(v2 - v1);
    if (len_sqr > max_len_sqr) 
    {
      max_len_sqr = len_sqr;
      max_edge_idx = i;
    }
  }
  op.width = static_cast<f32>(std::sqrt(max_len_sqr));

  // Calculate direction and rotation
  auto p1 = glm::dvec2(face.vertices[max_edge_idx]);
  auto p2 = glm::dvec2(face.vertices[(max_edge_idx + 1) % face.vertices.size()]);
  auto dir =glm::dvec2(glm::normalize(p2 - p1));
  op.rotation_z = static_cast<f32>(std::atan2(dir.y, dir.x));

  // Calculate thickness
  auto next_edge_idx = (max_edge_idx + 1) % face.vertices.size();
  auto p3 = face.vertices[(next_edge_idx + 1) % face.vertices.size()];
  op.thickness = static_cast<f32>(glm::distance(p2, p3));
  return op;
}




