#include <algorithm>
#include <memory>
#include <print>
#include <filesystem>
#include <stdexcept>
#include <vector>

#include "geometry.hpp"
#include "spatial_hashing.hpp"
#include "arrangement.hpp"
#include "io/drw_parser.hpp"
#include "io/gltf_exporter.hpp"
#include "graphics/mesh_visualizer.hpp"
#include "types.hpp"

#include <glm/trigonometric.hpp>
#include <glm/geometric.hpp>

// constexpr auto epsilon = static_cast<f64>(1e-4);
// constexpr auto ceil_H = 0.25f;
// constexpr auto wall_thickness = 0.125f;

void parse_cad(const std::filesystem::path& filename, 
               [[maybe_unused]] std::vector<Vertex_PN>& out_vertices, 
               [[maybe_unused]] std::vector<u32>& out_indices)
{
  // parsing DXF model to extract segments

  auto parser = DRWParser{};
  auto dxf = dxfRW(filename.string().c_str());
  if (!dxf.read(&parser, false))
    throw std::runtime_error(std::format("Error reading DXF file (code: {}): {}", static_cast<i32>(dxf.getError()), filename.string()));

  auto& wall_segments = parser.wall_segments;
  auto& door_segments = parser.door_segments;
  std::println("Successfully parsed DXF file! Wall segments: {}, door segments: {}", wall_segments.size(), door_segments.size());
  
  if(wall_segments.empty())
    throw std::runtime_error("No primitives found!");

  // detect the unit scale and normalize
  {
    auto segments_view = std::array<std::span<const Segment>, 2>{
      wall_segments, 
      door_segments
    };
    
    if(parser.unit_scale == 0.0f)
      parser.unit_scale = detect_unit_scale(segments_view | std::views::join);

    std::println("Unit scale: {}", parser.unit_scale);
    normalize_segments(parser.unit_scale, wall_segments);
    normalize_segments(parser.unit_scale, door_segments);
  }

   
  // Vertex snapping with spatial hashing data structure 
   
  {
    auto hash = SpatialHash{ 1e-6 };
    auto edges = std::vector<GraphEdge>{};
    auto all_segments = std::array<std::span<const Segment>, 2>{
      wall_segments, 
      door_segments
    };
    for (const auto& seg : all_segments | std::views::join)
    {
      auto v1 = hash.snap(seg.p1);
      auto v2 = hash.snap(seg.p2);
      if (v1 != v2)
        edges.push_back({ v1, v2, seg.layer });
    }
    
    auto& vertices = hash.vertices();
    std::println("Vertex snapping completed. Number of vertices: {}", vertices.size());
  }
 
  exit(0);
 
#if 0 

 
  // Arrangement + Halfedge
  
  auto arrangement = build_arrangement(vertices, edges);
  std::println("Arrangement successfully completed: vertices={}, edges={}, faces={}", arrangement.number_of_vertices(), arrangement.number_of_edges(), arrangement.number_of_faces());
  
  // face extraction

  auto faces = extract_faces(arrangement);
  std::println("Extracted faces: {}", faces.size());
  for (const auto& face : faces)
  {
    // Ensure CCW winding
    auto contour = face.vertices;
    if (calculate_signed_area(contour) < 0.0f)
      std::ranges::reverse(contour);
      
    auto p2t_points = std::vector<p2t::Point>{};
    auto p2t_ptr_points = std::vector<p2t::Point*>{};
    p2t_points.reserve(contour.size());
    p2t_ptr_points.reserve(contour.size());
    for (const auto& p : contour)
    {
      p2t_points.emplace_back(p2t::Point{ p.x, p.y });
      p2t_ptr_points.push_back(&p2t_points.back());
    }

    auto cdt = p2t::CDT{ p2t_ptr_points };
    cdt.Triangulate();
    auto floor_triangles = cdt.GetTriangles();
    
    build_floor(out_vertices, out_indices, floor_triangles);
    // build_ceil(out_vertices, out_indices, ceil_H, floor_triangles);
    
    // auto outer_wall = std::vector<glm::dvec2>{};
    // extrude_walls(out_vertices, out_indices, ceil_H, contour, outer_wall);
  }
#endif



#if 0
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
#endif
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
  
  auto vertices = std::vector<Vertex_PN>{};
  auto indices = std::vector<u32>{};
  if(is_load)
  {
    import_gltf(file_path, vertices, indices);
  }
  else if(is_parse)
  {
    parse_cad(file_path, vertices, indices);
   
    // exporting mesh in GLTF
    // auto gltf_path = file_path.filename().replace_extension("gltf");
    // std::println("Model will be exported to: {}", gltf_path.string());
    // export_to_gltf(vertices, indices, gltf_path);
  }


  // --- visualize mesh ---
  // ----------------------

  auto bbox = calculate_bounding_box_3D(vertices); 
  auto center = (bbox.min + bbox.max) * 0.5f;   
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