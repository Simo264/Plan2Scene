#include <algorithm>
#include <cstdlib>
#include <memory>
#include <print>
#include <filesystem>
#include <stdexcept>
#include <vector>

#include "geometry.hpp"
#include "io/drw_parser.hpp"
#include "io/gltf_exporter.hpp"
#include "graphics/mesh_visualizer.hpp"

#include <poly2tri/sweep/cdt.h>
#include <glm/trigonometric.hpp>
#include <glm/geometric.hpp>

constexpr auto epsilon = static_cast<f64>(1e-4);

void build_floor(std::vector<Vertex>& out_vertices, 
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

      auto v = Vertex{};
      v.position.x = static_cast<f32>(p->x);
      v.position.y = 0.f;
      v.position.z = static_cast<f32>(p->y);
      v.normal = {0.f, 1.f, 0.f},
      out_vertices.push_back(v);
    }
  }
}

void build_ceil(std::vector<Vertex>& out_vertices, 
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

      auto v = Vertex{};
      v.position.x = static_cast<f32>(p->x);
      v.position.y = H;
      v.position.z = static_cast<f32>(p->y);
      v.normal = {0.f, -1.f, 0.f},  // down, into the room
      out_vertices.push_back(v);
    }
  }
}

void extrude_walls(std::vector<Vertex>& vertices, 
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

    auto BL = Vertex{ .position={f32(p1.x), 0.f, f32(p1.y)}, .normal=normal };
    auto BR = Vertex{ .position={f32(p2.x), 0.f, f32(p2.y)}, .normal=normal };
    auto TR = Vertex{ .position={f32(p2.x), H, f32(p2.y)},   .normal=normal };
    auto TL = Vertex{ .position={f32(p1.x), H, f32(p1.y)},   .normal=normal };

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

    auto BL = Vertex{ .position={f32(p1.x), 0.f, f32(p1.y)}, .normal=normal };
    auto BR = Vertex{ .position={f32(p2.x), 0.f, f32(p2.y)}, .normal=normal };
    auto TR = Vertex{ .position={f32(p2.x), H, f32(p2.y)},   .normal=normal };
    auto TL = Vertex{ .position={f32(p1.x), H, f32(p1.y)},   .normal=normal };

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

void build_wall_top_cap(std::vector<Vertex>& out_vertices,
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

      auto v = Vertex{};
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

void parse_cad(const std::filesystem::path& filename, 
               std::vector<Vertex>& out_vertices, 
               std::vector<u32>& out_indices)
{
  // --- Step 1: parsing DXF file to retrieve segments and polylines ---
  // -------------------------------------------------------------------
  auto parser = DRWParser{};
  auto dxf = dxfRW(filename.string().c_str());
  if (!dxf.read(&parser, false))
    throw std::runtime_error(std::format("Error reading DXF file (code: {}): {}", static_cast<i32>(dxf.getError()), filename.string()));

  std::println("Successfully parsed DXF file: segments: {}, polylines: {}", parser.wall_segments.size(), parser.wall_polylines.size());
  exit(0);

  if(parser.wall_polylines.empty())
    throw std::runtime_error("No wall polyline found");

  // With polylines we already have an ordered contour.
  auto& wall_polyline = parser.wall_polylines.front();
  std::println("Wall polyline has {} points.", wall_polyline.points.size());  

  // Is polyline closed: we should check the distance between them v[0] and v[last] and if their 
  // distance is less than epsilon they represent the same logical point. 
  // We can drop the last vertex so the contour doesn't have a near-duplicate.
  if(wall_polyline.closed)
  {
    auto first_point = wall_polyline.points.front();
    auto last_point = wall_polyline.points.back();
    std::println("Polyline is closed. First point: ({}, {}), last point: ({}, {})", first_point.x, first_point.y, last_point.x, last_point.y);
    if(glm::distance(first_point, last_point) < epsilon)
    {
      std::println("Merge the first and last points");
      wall_polyline.points.pop_back();
    }
  }
  else 
  {
    // Polyline is open: we should check if the first and last point are close enough to be considered the same point.
    auto first_point = wall_polyline.points.front();
    auto last_point = wall_polyline.points.back();
    std::println("Polyline is not closed. First point: ({}, {}), last point: ({}, {})", first_point.x, first_point.y, last_point.x, last_point.y);
    if(glm::distance(first_point, last_point) < epsilon)
    {
      std::println("First and last point are closer than epsilon. Drop the last point to avoid near-duplicate and consider it as closed.");
      wall_polyline.points.pop_back();
      wall_polyline.closed = true;
    }
    else 
      throw std::runtime_error("First and last point are not closer than epsilon. Exit with error because we need a closed contour for triangulation.");
  }

  if(parser.unit_scale == 0.0f)
  {
    std::println("Unit scale not specified in DXF header. Detecting unit scale from geometry...");
    parser.unit_scale = detect_unit_scale(wall_polyline.points);
    std::println("Detected unit scale: {}", parser.unit_scale);
  }

  for (auto& p : wall_polyline.points)
  {
    p.x *= parser.unit_scale;
    p.y *= parser.unit_scale;
  } 
  
  // --- Step 2: triangulation of the contour using poly2tri ---
  // ----------------------------------------------------------
  // poly2tri expects the outer polygon to be counter-clockwise (CCW) and holes to be clockwise (CW).
  // If signed_area < 0 the order is CW: we must reverse the vertices before passing to poly2tri.
  auto area = calculate_signed_area(wall_polyline);
  std::println("Signed area of the contour: {}", area);
  if(area < 0.0f)
    std::ranges::reverse(wall_polyline.points);
   
  auto floor_contour = std::vector<p2t::Point*>{};
  floor_contour.reserve(wall_polyline.points.size());
  for(const auto& p : wall_polyline.points)
    floor_contour.push_back(new p2t::Point{p.x, p.y});
    
  auto cdt = p2t::CDT(floor_contour);
  cdt.Triangulate();
  auto floor_triangles = cdt.GetTriangles();
  std::println("Triangulation completed. Number of floor triangles: {}", floor_triangles.size());

  // --- Step 3: extrusion and mesh creation ---
  // -------------------------------------------
  constexpr auto ceil_H = 3.f;
  constexpr auto wall_thickness = 0.125f;

  auto& inner_wall = wall_polyline.points;
  auto outer_wall = compute_polygon_offsetting(inner_wall, wall_thickness);

  // reserve memory for vertices and indices
  auto nr_vertices_floor = floor_triangles.size() * 3;
  auto nr_vertices_wall = inner_wall.size() * 4   // inner faces
                                      + outer_wall.size() * 4;         // outer faces
  auto nr_vertices_top_cap = (inner_wall.size() + outer_wall.size()) * 3;
  auto nr_vertices_ceil = nr_vertices_floor;
  out_vertices.reserve(nr_vertices_floor + nr_vertices_wall + nr_vertices_top_cap + nr_vertices_ceil);
  
  auto nr_indices_floor    = floor_triangles.size() * 3;
  auto nr_indices_walls    = inner_wall.size() * 6    // inner quads
                                        + outer_wall.size() * 6;    // outer quads
  auto nr_indices_top_cap  = (inner_wall.size() + outer_wall.size()) * 3;
  auto nr_indices_ceil     = floor_triangles.size() * 3;
  out_indices.reserve(nr_indices_floor + nr_indices_walls + nr_indices_top_cap + nr_indices_ceil);  

  build_floor(out_vertices, out_indices, floor_triangles);
  build_ceil(out_vertices, out_indices, ceil_H, floor_triangles);
  // Clean up poly2tri points.
  for (auto* pt : floor_contour) delete pt;

  // takes both inner and outer polygons
  extrude_walls(out_vertices, out_indices, ceil_H, inner_wall, outer_wall);
  build_wall_top_cap(out_vertices, out_indices, ceil_H, inner_wall, outer_wall);
}

int main(int argc, char* argv[])
{
  if(argc != 3)
    throw std::runtime_error("Usage:\n1. /build/Plan2Scene --load <model/input.gltf>\n2. ./build/Plan2Scene --parse <cad/input.dxf>");
  
  auto mode = std::string(argv[1]);
  auto is_parse = mode == "--parse";
  auto is_load = mode == "--load";
  if(!is_load && ! is_parse)
    throw std::runtime_error(
      "Usage example:\n 1. /build/Plan2Scene --load <model/input.gltf>\n2. ./build/Plan2Scene --parse <cad/input.dxf>");

  auto file_path = std::filesystem::path(argv[2]);
  if(!std::filesystem::exists(file_path))
    throw std::runtime_error(std::format("Input file not found: {}", file_path.string()));
  
  auto vertices = std::vector<Vertex>{};
  auto indices = std::vector<u32>{};
  if(is_load)
  {
    import_gltf(file_path, vertices, indices);
  }
  else if(is_parse)
  {
    parse_cad(file_path, vertices, indices);
   
    // exporting mesh in GLTF
    auto gltf_path = file_path.filename().replace_extension("gltf");
    std::println("Model will be exported to: {}", gltf_path.string());
    export_to_gltf(vertices, indices, gltf_path);
  }


  // --- visualize mesh ---
  // ----------------------

  // Get the bounds of the extruded 3D room
  auto bbox = calculate_bounding_box(vertices);
  auto center = (bbox.min + bbox.max) * 0.5f; 
  // Center the model at the origin (0, 0, 0)
  auto transform = Transformation{};
  transform.position = -center;
  transform.update_tranformation();
  
  auto visualizer = MeshVisualizer(1024, 768);
  visualizer.set_mesh(std::make_shared<StaticMesh>(
    vertices.data(), 
    vertices.size(),
    indices.data(),  
    indices.size()
  ));
  visualizer.set_mesh_transform(transform);
  visualizer.camera().eye = { 0.f, 2.f, 10.f };
  visualizer.camera().set_orientation(glm::radians(glm::vec3{ -5.f, 0.f, 0.f }));
  visualizer.render();
  return 0;
}