#include "geometry.hpp"
#include "types.hpp"
#include "globals.hpp"

#include "dbscan.h"

#include <numeric>

#include <glm/ext/vector_double2.hpp>
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

BoundingBox2D calculate_bbox_2D(const Face& face)
{
  const auto& polyline = face.vertices;
  return calculate_bbox_2D(polyline);
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

BoundingBox3D calculate_bbox_3D(const std::vector<Vertex_PNT>& vertices) 
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

glm::dvec2 calculate_center(const Face& face)
{
  auto sum = std::accumulate(face.vertices.begin(), face.vertices.end(), glm::dvec2(0.0));
  return sum / static_cast<double>(face.vertices.size());
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

  throw std::runtime_error(std::format(
    "Failed to close wall gap:\n gap_start=({:.4f}, {:.4f}), gap_end=({:.4f}, {:.4f}).\n"
    "No parallel projection found for B={}, C={}, D={}, E={}.",
    gap_start.x, gap_start.y, gap_end.x, gap_end.y,
    wall_vertices.B, wall_vertices.C, wall_vertices.D, wall_vertices.E
  ));
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
      close_wall_gap(door.start, door.end, LayerType::DOOR, hash, edges);
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
        
    auto box = calculate_bbox_2D(sample_points, cluster_indices);
    auto sides = get_long_sides_bbox2d(box);
    auto longest_side = sides.at(0);
    try 
    {
      close_wall_gap(longest_side.start, longest_side.end, LayerType::WINDOW, hash, edges);
    } 
    catch (const std::exception& e)
    {
      throw std::runtime_error(std::format("Window reconstruction failed at cluster index {} (of {}):\n{}",i, clusters.size(), e.what()));
    }
  }
}

void ensure_winding_matches_normal(Vertex_PNT& v0, 
                                   Vertex_PNT& v1, 
                                   Vertex_PNT& v2, 
                                   const glm::vec3& desired_normal)
{
  auto geometric_normal = glm::cross(v1.position - v0.position, v2.position - v0.position);
  if (glm::dot(geometric_normal, desired_normal) < 0.0f)
    std::swap(v1, v2);
}

void build_triangulated_face(std::vector<Vertex_PNT>& out_vertices,
                            std::vector<u32>& out_indices,
                            const std::vector<p2t::Triangle*> triangles,
                            f32 height,
                            bool facing_up,
                            const BoundingBox2D& face_bbox)
{ 
  auto desired_normal = facing_up ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(0.0f, -1.0f, 0.0f);
  for (const auto& tri : triangles) 
  { 
    auto verts = std::array<Vertex_PNT, 3>{};
    for (auto i = 0; i < 3; ++i)  
    {  
      auto p = tri->GetPoint(i); 
      auto v = Vertex_PNT{}; 
      v.position.x = static_cast<f32>(p->x); 
      v.position.y = height; 
      v.position.z = static_cast<f32>(p->y); 
      v.normal = desired_normal;
      v.text_coord.x = (v.position.x - face_bbox.min.x) / g_config.floor_texture_scaling; 
      v.text_coord.y = (v.position.z - face_bbox.min.y) / g_config.floor_texture_scaling; 
      verts[i] = v;
    }

    ensure_winding_matches_normal(verts[0], verts[1], verts[2], desired_normal);

    for (const auto& v : verts)
    {
      auto idx = static_cast<u32>(out_vertices.size()); 
      out_vertices.push_back(v); 
      out_indices.push_back(idx); 
    }
  } 
}

void triangulate_face(std::vector<Vertex_PNT>& out_vertices,
                      std::vector<u32>& out_indices,
                      f32 height,
                      bool facing_up, 
                      const Face& face)
{
  auto polyline = face.vertices;
  auto face_bbox = calculate_bbox_2D(polyline);
    
  auto p2t_points = std::vector<p2t::Point>{};
  auto p2t_ptr_points = std::vector<p2t::Point*>{};
  p2t_points.reserve(polyline.size());
  p2t_ptr_points.reserve(polyline.size());
  for (const auto& p : polyline)
  {
    p2t_points.emplace_back(p2t::Point{ p.x, p.y });
    p2t_ptr_points.push_back(&p2t_points.back());
  }
  
  auto cdt = p2t::CDT{ p2t_ptr_points };
  cdt.Triangulate();
  auto triangles = cdt.GetTriangles();

  build_triangulated_face(out_vertices, out_indices, triangles, height, facing_up, face_bbox);
}

void extrude_face(std::vector<Vertex_PNT>& vertices,
                  std::vector<u32>& out_indices,
                  f32 base_height,
                  f32 top_height,
                  const Face& face)
{
  constexpr glm::vec3 up(0.0f, 1.0f, 0.0f); 
  const auto& contour = face.vertices; 
  auto v_bottom_uv = base_height / g_config.wall_texture_scaling; 
  auto v_top_uv = top_height / g_config.wall_texture_scaling; 

  for (auto i = 0u; i < contour.size(); ++i)  
  { 
    auto p1 = contour[i]; 
    auto p2 = contour[(i + 1) % contour.size()]; 
    auto edge = glm::vec3(f32(p2.x - p1.x), 0.0f, f32(p2.y - p1.y)); 
    auto edge_len = glm::length(edge); 
    auto normal = glm::normalize(glm::cross(up, edge)); 
    auto u0 = 0.0f; 
    auto u1 = edge_len / g_config.wall_texture_scaling; 

    auto BL = Vertex_PNT{ {f32(p1.x), base_height, f32(p1.y)}, normal, {u0, v_bottom_uv} }; 
    auto BR = Vertex_PNT{ {f32(p2.x), base_height, f32(p2.y)}, normal, {u1, v_bottom_uv} }; 
    auto TR = Vertex_PNT{ {f32(p2.x), top_height, f32(p2.y)}, normal, {u1, v_top_uv} }; 
    auto TL = Vertex_PNT{ {f32(p1.x), top_height, f32(p1.y)}, normal, {u0, v_top_uv} }; 

    auto tri_normal = glm::cross(BR.position - BL.position, TR.position - BL.position);
    auto winding_ok = glm::dot(tri_normal, normal) >= 0.0f;
    
    auto base = static_cast<u32>(vertices.size()); 
    vertices.push_back(BL); 
    vertices.push_back(BR); 
    vertices.push_back(TR); 
    vertices.push_back(TL); 
    if (winding_ok)
    {
      out_indices.push_back(base + 0);
      out_indices.push_back(base + 1);
      out_indices.push_back(base + 2);
      out_indices.push_back(base + 0);
      out_indices.push_back(base + 2);
      out_indices.push_back(base + 3);
    }
    else
    {
      out_indices.push_back(base + 0);
      out_indices.push_back(base + 2);
      out_indices.push_back(base + 1);
      out_indices.push_back(base + 0);
      out_indices.push_back(base + 3);
      out_indices.push_back(base + 2);
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

  auto center_2d = calculate_center(face);
  op.height = z_max - z_min;
  op.center = glm::vec3(center_2d.x, z_min + op.height * 0.5, center_2d.y);

  // Find longest side of the face
  auto max_edge_idx = 0;
  auto max_len_sqr = 0.0;
  for (size_t i = 0; i < face.vertices.size(); ++i) 
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



void center_mesh(std::vector<Vertex_PNT>& vertices, std::vector<OpeningInstance>& openings)
{
  auto bbox = calculate_bbox_3D(vertices);
  auto center = (bbox.min + bbox.max) * 0.5f;
  for (auto& v : vertices) 
  {
    v.position.x -= center.x;
    v.position.y -= center.y;
    v.position.z -= center.z;
  }

  for (auto& o : openings)
  {
    o.center.x -= center.x;
    o.center.y -= center.y;
    o.center.z -= center.z;
  }
}
