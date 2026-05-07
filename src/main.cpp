#include <algorithm>
#include <memory>
#include <print>
#include <filesystem>
#include <stdexcept>
#include <vector>

#include "geometry.hpp"
#include "glm/trigonometric.hpp"
#include "io/drw_parser.hpp"
#include "io/gltf_exporter.hpp"
#include "graphics/mesh_visualizer.hpp"

#include <poly2tri/sweep/cdt.h>
#include <glm/geometric.hpp>

constexpr auto epsilon = static_cast<f64>(1e-4);

void build_floor(std::vector<Vertex>& vertices, const std::vector<p2t::Triangle*> triangle_list)
{
  for (const auto& tri : triangle_list)
  {
    for (auto i = 0; i < 3; ++i)
    {
      auto p = tri->GetPoint(i);
      auto v = Vertex{};
      v.position.x = static_cast<f32>(p->x);
      v.position.y = 0.f; // floor at y=0
      v.position.z = static_cast<f32>(p->y);
      v.normal = {0.f, 1.f, 0.f}; // normal points up (+Y)
      vertices.push_back(v);
    }
  }
}

void build_ceil(std::vector<Vertex>& vertices, f32 H, const std::vector<p2t::Triangle*> triangle_list)
{
  for (const auto& tri : triangle_list)
  {
    for (auto i = 0; i < 3; ++i)
    {
      auto p = tri->GetPoint(i);
      auto v = Vertex{};
      v.position.x = static_cast<f32>(p->x);
      v.position.y = H; // ceiling at y=H
      v.position.z = static_cast<f32>(p->y);
      v.normal = {0.f, -1.f, 0.f}; // normal points down (-Y)
      vertices.push_back(v);
    }
  }
}

void extrude_walls(std::vector<Vertex>& vertices, f32 H, const std::vector<glm::dvec2>& wall_points)
{
  for (auto i = 0u; i < wall_points.size(); ++i)
  {
    auto p1 = wall_points.at(i);
    auto p2 = wall_points.at((i + 1) % wall_points.size());

    // outward normal: edge direction in XZ plane rotated 90 degrees
    auto dx = f32(p2.x - p1.x);
    auto dz = f32(p2.y - p1.y);
    auto len = std::sqrt(dx * dx + dz * dz);
    auto normal = glm::vec3{dz / len, 0.f, -dx / len};

    // 4 corners of the wall quad, Y-up convention
    auto BL = Vertex{ .position={static_cast<f32>(p1.x), 0.f,  static_cast<f32>(p1.y)}, .normal=normal};
    auto BR = Vertex{ .position={static_cast<f32>(p2.x), 0.f,  static_cast<f32>(p2.y)}, .normal=normal};
    auto TR = Vertex{ .position={static_cast<f32>(p2.x), H,    static_cast<f32>(p2.y)}, .normal=normal};
    auto TL = Vertex{ .position={static_cast<f32>(p1.x), H,    static_cast<f32>(p1.y)}, .normal=normal}; 
    // triangle 1: BL, BR, TR
    vertices.push_back(BL);
    vertices.push_back(BR);
    vertices.push_back(TR);
    // triangle 2: BL, TR, TL
    vertices.push_back(BL);
    vertices.push_back(TR);
    vertices.push_back(TL);
  } 
}


int main(int argc, char* argv[]) 
{
  if(argc < 2)
    throw std::runtime_error(std::format("No input file provided. Usage: {} <input.dxf>", argv[0]));
  
  auto file_path = argv[1];
  if(!std::filesystem::exists(file_path))
    throw std::runtime_error(std::format("Input file not found: {}", file_path));
  
  // --- Step 1: parsing DXF file to retrieve segments and polylines ---
  // -------------------------------------------------------------------
  auto parser = DRWParser{};
  auto dxf = dxfRW(file_path);
  if (!dxf.read(&parser, false))
    throw std::runtime_error(std::format("Error reading DXF file (code: {}): {}", static_cast<int>(dxf.getError()), file_path));

  auto& segments = parser.segments;
  auto& polylines = parser.polylines;
  std::println("Successfully parsed DXF file: segments: {}, polylines: {}", segments.size(), polylines.size());

  auto& wall_polyline = polylines.front();
  auto& wall_points = wall_polyline.points;
  std::println("Wall polyline has {} points.", wall_points.size());

  if(parser.unit_scale == 0.0f)
  {
    std::println("Unit scale not specified in DXF header. Detecting unit scale from geometry...");
    parser.unit_scale = detect_unit_scale(wall_points);
    std::println("Detected unit scale: {}", parser.unit_scale);
  }

  for (auto& p : wall_points)
  {
    p.x *= parser.unit_scale;
    p.y *= parser.unit_scale;
  }
  
  // We have unordered disconnected segments?
  // The triangulation library needs an ordered sequence of vertices forming a closed polygon.
  // We must convert this unordered segments into ordered closed contour.
  if(!segments.empty())
  {
    //for(const auto& seg : segments)
    //  std::println("Segment: p1 =({}, {}), p2 = ({}, {})", seg.p1.x, seg.p1.y, seg.p2.x, seg.p2.y);
    
    // espilon merging of points to merge segments that are close enough to be considered connected.
    // Two points closer than epsilon become the same point.
    std::println("todo: merging points...");
    // Once points are snapped, we must build an adjacency graph
    std::println("todo: chaining segments...");
    throw std::runtime_error("Chaining segments into a closed contour is not implemented yet.");
  }
  // We have polylines? Then we already have an ordered contour.
  else if(!polylines.empty())
  {
    // Is polyline closed: we should check the distance between them v[0] and v[last] and if their 
    // distance is less than epsilon they represent the same logical point. 
    // We can drop the last vertex so the contour doesn't have a near-duplicate.
    if(wall_polyline.closed)
    {
      auto first_point = wall_points.front();
      auto last_point = wall_points.back();
      std::println("Polyline is closed. First point: ({}, {}), last point: ({}, {})", first_point.x, first_point.y, last_point.x, last_point.y);
      if(glm::distance(first_point, last_point) < epsilon)
      {
        std::println("First and last point are closer than epsilon. Drop the last point to avoid near-duplicate.");
        wall_points.pop_back();
      }
    }
    // Polyline is open: we should check if the first and last point are close enough to be 
    // considered the same point.
    else 
    {
      auto first_point = wall_points.front();
      auto last_point = wall_points.back();
      std::println("Polyline is not closed. First point: ({}, {}), last point: ({}, {})", first_point.x, first_point.y, last_point.x, last_point.y);
      if(glm::distance(first_point, last_point) < epsilon)
      {
        std::println("First and last point are closer than epsilon. Drop the last point to avoid near-duplicate and consider it as closed.");
        wall_points.pop_back();
        wall_polyline.closed = true;
      }
      else 
        throw std::runtime_error("First and last point are not closer than epsilon. Exit with error because we need a closed contour for triangulation.");
    }
  }
  
  // --- Step 2: triangulation of the contour using poly2tri---
  // ----------------------------------------------------------
  // Compute signed area to determine orientation
  // poly2tri expects the outer polygon to be counter-clockwise (CCW) and holes to be clockwise (CW).
  // If signed_area < 0 the order is CW: we must reverse the vertices before passing to poly2tri.
  // Otherwise, if signed_area > 0 the order is CCW and we can pass the vertices as they are.
  if (wall_points.size() < 3)
    throw std::runtime_error("Not enough points to triangulate");
  
  auto area = calculate_signed_area(wall_polyline);
  std::println("Signed area of the contour: {}", area);
  if(area < 0)
    std::ranges::reverse(wall_points);
   
  auto contour = std::vector<p2t::Point*>{};
  contour.reserve(wall_points.size());
  for(const auto& p : wall_points)
    contour.push_back(new p2t::Point{p.x, p.y});
    
  auto cdt = p2t::CDT(contour);
  cdt.Triangulate();
  auto triangle_list = cdt.GetTriangles();
  std::println("Triangulation completed. Number of triangles: {}", triangle_list.size());

  // --- Step 3: extrusion and mesh creation ---
  // -------------------------------------------

  // Define the vertices for our mesh
  auto nr_vertices_floor = triangle_list.size() * 3;
  auto nr_vertices_wall = wall_points.size() * 6; // 2 triangles * 3 vertices per edge
  auto nr_vertices_ceil = nr_vertices_floor; // same as floor
  auto vertices = std::vector<Vertex>{};
  vertices.reserve(nr_vertices_floor + nr_vertices_wall + nr_vertices_ceil);
  
  // build the floor
  build_floor(vertices, triangle_list);
  
  // wall extrusion
  constexpr auto H = 3.f; // 3 meters
  extrude_walls(vertices, H, wall_points);
  
  // build the ceiling (same triangles as floor but at height H and normal pointing down)
  build_ceil(vertices, H, triangle_list);

//#define VISUALIZE_MESH
#ifdef VISUALIZE_MESH 
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
    nullptr,  
    0
  ));
  visualizer.set_mesh_transform(transform);
  visualizer.camera().eye = { 0.f, 10.f, 30.f };
  visualizer.camera().set_orientation(glm::radians(glm::vec3{ -10.f, 0.f, 0.f })); // look slightly down
  visualizer.render();
#endif

  // --- Step 4: exporting mesh in GLTF ---
  // --------------------------------------
  auto model_path = "output_model.gltf";
  std::println("Model will be exported to: {}", model_path);
  export_to_gltf(vertices, model_path);  
  return 0;
}