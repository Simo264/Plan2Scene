#include "geometry.hpp"

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

auto calculate_bounding_box(const std::vector<glm::dvec2>& points) -> BoundingBox 
{ 
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

auto calculate_bounding_box(const std::vector<Vertex_PN>& vertices) -> BoundingBox 
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

void normalize_vertices(f32 unit_scale, std::vector<glm::dvec2>& vertices)
{
  if(unit_scale == 0.0f)
    unit_scale = detect_unit_scale(vertices);

  std::println("Unit scale: {}", unit_scale);
  
  for (auto i = 0u; i < vertices.size(); i++)
    vertices.at(i) *= unit_scale;

  auto bb = calculate_bounding_box(vertices);
  auto center = (bb.min + bb.max) * 0.5f;
  std::println("Geometry bounds: min=({}, {}, {}) max=({}, {}, {})", bb.min.x, bb.min.y, bb.min.z, bb.max.x, bb.max.y, bb.max.z);
  std::println("Geometry center: {}, {}, {}", center.x, center.y, center.z);
}

auto compute_polygon_offsetting(const std::vector<glm::dvec2>& inner_points, 
                                f32 thickness) -> std::vector<glm::dvec2>
{
  // Convert to Clipper2 format
  auto innerPath = Clipper2Lib::PathD{};
  for (const auto& p : inner_points) 
    innerPath.push_back(Clipper2Lib::PointD(p.x, p.y));

  // Inflate the polygon outward
  Clipper2Lib::PathsD solution = InflatePaths(
      Clipper2Lib::PathsD{innerPath}, 
      thickness, 
      Clipper2Lib::JoinType::Miter, // Good for architectural corners
      Clipper2Lib::EndType::Polygon // Closed polygon
  );

  // Convert back to glm::dvec2
  auto outer_points = std::vector<glm::dvec2>{};
  if (!solution.empty() && !solution[0].empty()) 
  {
    for (const auto& pt : solution[0]) 
    {
      outer_points.emplace_back(pt.x, pt.y);
    }
  }
  
  return outer_points;
}

void build_floor(std::vector<Vertex_PN>& out_vertices, 
                 std::vector<u32>& out_indices,
                 const std::vector<p2t::Triangle*> floor_triangles)
{
  for (const auto& tri : floor_triangles)
  {
    for (auto i = 0; i < 3; ++i) 
    {
      // poly2tri gives CCW winding in XY, so we emit points in order 0,1,2 which stays CCW
      auto p = tri->GetPoint(i);
      auto idx = static_cast<u32>(out_vertices.size());
      out_indices.push_back(idx);

      auto v = Vertex_PN{};
      v.position.x = static_cast<f32>(p->x);
      v.position.y = 0.f;
      v.position.z = static_cast<f32>(p->y);
      v.normal = {0.f, 1.f, 0.f},
      out_vertices.push_back(v);
    }
  }
}

void build_ceil(std::vector<Vertex_PN>& out_vertices, 
                std::vector<u32>& out_indices,
                f32 H,
                const std::vector<p2t::Triangle*> triangle_list)
{
  // The ceiling is almost identical to the floor with y = H and normal {0, -1, 0}.
  // However there's one subtle but important issue: the winding order must be reversed.
  for (const auto& tri : triangle_list)
  {
    for (auto i = 2; i >= 0; --i) 
    {
      auto p = tri->GetPoint(i);
      auto idx = static_cast<u32>(out_vertices.size());
      out_indices.push_back(idx);

      auto v = Vertex_PN{};
      v.position.x = static_cast<f32>(p->x);
      v.position.y = H;
      v.position.z = static_cast<f32>(p->y);
      v.normal = {0.f, -1.f, 0.f},  // down, into the room
      out_vertices.push_back(v);
    }
  }
}

void extrude_walls(std::vector<Vertex_PN>& vertices, 
                   std::vector<u32>& out_indices,
                   f32 H,
                   const std::vector<glm::dvec2>& inner_points,
                   const std::vector<glm::dvec2>& outer_points) 
{
  constexpr glm::vec3 up(0.0f, 1.0f, 0.0f);
  
  // Extrude inner walls (with normals pointing inward)
  for (auto i = 0u; i < inner_points.size(); ++i) 
  {
    auto p1 = inner_points.at(i);
    auto p2 = inner_points.at((i + 1) % inner_points.size());

    // The inner face must have normals pointing inward (into the room). 
    // For a CCW contour, the cross(edge, up) points outward, so we need to negate it. 
    // And the winding must be reversed to match:
    auto edge = glm::vec3(f32(p2.x - p1.x), 0.0f, f32(p2.y - p1.y));
    auto normal = -glm::normalize(glm::cross(edge, up));

    auto BL = Vertex_PN{ .position={f32(p1.x), 0.f, f32(p1.y)}, .normal=normal };
    auto BR = Vertex_PN{ .position={f32(p2.x), 0.f, f32(p2.y)}, .normal=normal };
    auto TR = Vertex_PN{ .position={f32(p2.x), H, f32(p2.y)},   .normal=normal };
    auto TL = Vertex_PN{ .position={f32(p1.x), H, f32(p1.y)},   .normal=normal };

    auto base = static_cast<u32>(vertices.size());
    vertices.push_back(BL);
    vertices.push_back(BR);
    vertices.push_back(TR);
    vertices.push_back(TL);

    out_indices.push_back(base + 0);
    out_indices.push_back(base + 2);
    out_indices.push_back(base + 1);
    out_indices.push_back(base + 0);
    out_indices.push_back(base + 3);
    out_indices.push_back(base + 2);
  }

  // Extrude Outer walls (with normals pointing outward)
  for (auto i = 0u; i < outer_points.size(); ++i) 
  {
    auto p1 = outer_points[i];
    auto p2 = outer_points[(i + 1) % outer_points.size()];

    // The outer polygon is also CCW (Clipper2 preserves winding), so cross(edge, up) points outward 
    auto edge = glm::vec3(f32(p2.x - p1.x), 0.0f, f32(p2.y - p1.y));
    auto normal = glm::normalize(glm::cross(edge, up));

    auto BL = Vertex_PN{ .position={f32(p1.x), 0.f, f32(p1.y)}, .normal=normal };
    auto BR = Vertex_PN{ .position={f32(p2.x), 0.f, f32(p2.y)}, .normal=normal };
    auto TR = Vertex_PN{ .position={f32(p2.x), H, f32(p2.y)},   .normal=normal };
    auto TL = Vertex_PN{ .position={f32(p1.x), H, f32(p1.y)},   .normal=normal };

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

void build_wall_top_cap(std::vector<Vertex_PN>& out_vertices,
                        std::vector<u32>& out_indices,
                        f32 H,
                        const std::vector<glm::dvec2>& inner_points,
                        const std::vector<glm::dvec2>& outer_points)
{
  // Build the outer contour (CCW) for poly2tri.
  auto outer_contour = std::vector<p2t::Point*>{};
  outer_contour.reserve(outer_points.size());
  for (const auto& p : outer_points)
    outer_contour.push_back(new p2t::Point(p.x, p.y));

  // Build the inner hole (must be CW, opposite to the outer CCW contour).
  // Since inner_points is CCW, we reverse it.
  auto inner_hole = std::vector<p2t::Point*>{};
  inner_hole.reserve(inner_points.size());
  for (auto it = inner_points.rbegin(); it != inner_points.rend(); ++it)
    inner_hole.push_back(new p2t::Point(it->x, it->y));

  auto cdt = p2t::CDT(outer_contour);
  cdt.AddHole(inner_hole);
  cdt.Triangulate();

  auto triangle_list = cdt.GetTriangles();
  std::println("Wall top cap triangulation: {} triangles", triangle_list.size());

  // Emit triangles at y = H with normals pointing up.
  for (const auto& tri : triangle_list)
  {
    for (auto i = 0; i < 3; ++i)
    {
      auto p = tri->GetPoint(i);
      auto idx = static_cast<u32>(out_vertices.size());
      out_indices.push_back(idx);

      auto v = Vertex_PN{};
      v.position.x = static_cast<f32>(p->x);
      v.position.y = H;
      v.position.z = static_cast<f32>(p->y);
      v.normal = {0.f, 1.f, 0.f};  // up
      out_vertices.push_back(v);
    }
  }

  // Clean up poly2tri points.
  for (auto* pt : outer_contour) delete pt;
  for (auto* pt : inner_hole)  delete pt;
}