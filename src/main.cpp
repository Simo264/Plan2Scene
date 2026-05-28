#include <algorithm>
#include <array>
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

// constexpr auto wall_thickness = 0.125f;

void dump_wall_vertices(const std::vector<Segment>& wall_segments)
{
  std::println("wall_segments = [");
  for (const auto& seg : wall_segments)
    std::println("\t(({}, {}), ({}, {})),", seg.p1.x, seg.p1.y, seg.p2.x, seg.p2.y);
  std::println("\n]");
}
void dump_door_vertices(const Segment& door_segment)
{
  const auto& p1 = door_segment.p1;
  const auto& p2 = door_segment.p2;
  std::println("door_segment = ( ({},{}), ({}, {}) )", p1.x, p1.y, p2.x, p2.y);
}

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

  // detect the unit scale and normalize
   
  {
    auto segments_view = std::array{ wall_segments };
    
    if(parser.unit_scale == 0.0f)
      parser.unit_scale = detect_unit_scale(segments_view | std::views::join);

    std::println("Unit scale: {}", parser.unit_scale);
    normalize_segments(parser.unit_scale, wall_segments);
    normalize_segments(parser.unit_scale, door_segments);
  }

  // Vertex snapping with spatial hashing data structure: wall segments only

  auto hash = SpatialHash{ 1e-6 };
  auto edges = std::vector<GraphEdge>{};
  auto segments_view = std::array{ wall_segments };
  for (const auto& seg : segments_view | std::views::join)
  {
    auto v1 = hash.snap(seg.p1);
    auto v2 = hash.snap(seg.p2);
    if (v1 != v2)
      edges.push_back({ v1, v2, seg.layer });
  }
  auto& vertices = hash.vertices();
  std::println("Vertex snapping completed. Number of vertices: {}", vertices.size());

  // dump_wall_vertices(wall_segments);
  // dump_door_vertices(door_seg);

  // Insert two edges for the door
  
  {
    auto& door_seg = door_segments.front();
    auto A_id = hash.find_nearest(door_seg.p1);
    auto B_id = hash.find_nearest(door_seg.p2);
    auto wall_dir = glm::normalize(vertices[A_id] - vertices[B_id]);
    door_seg.p1 = vertices[A_id];
    door_seg.p2 = vertices[B_id];
    // dump_door_vertices(door_seg);

    auto nbrs_A = find_neighboors(A_id, edges);
    auto A_prime_id = get_adjacent_vertex(wall_dir, A_id, nbrs_A, vertices);
    // auto A = vertices[A_id];
    // auto A_prime = vertices[A_prime_id];
    // std::println("A: ({}, {}) A': ({}, {})", A.x, A.y, A_prime.x, A_prime.y);
    
    auto nbrs_B = find_neighboors(B_id, edges);
    auto B_prime_id = get_adjacent_vertex(wall_dir, B_id, nbrs_B, vertices);
    // auto B = vertices[B_id];
    // auto B_prime = vertices[B_prime_id];
    // std::println("B: ({}, {}) B': ({}, {})", B.x, B.y, B_prime.x, B_prime.y);

    edges.push_back(GraphEdge{ A_id, B_id, LayerType::DOOR });
    edges.push_back(GraphEdge{ A_prime_id, B_prime_id, LayerType::DOOR });
  }

  // Arrangement + Halfedge
  
  auto arrangement = build_arrangement(vertices, edges);
  std::println("Arrangement successfully completed: vertices={}, edges={}, faces={}", 
    arrangement.number_of_vertices(), arrangement.number_of_edges(), arrangement.number_of_faces());
  
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
    auto triangles = cdt.GetTriangles();
    
    //constexpr auto ceil_height = 10.f;
    switch(face.type)
    {
      case FaceType::ROOM:
        std::println("Room face found!");
        build_triangulated_face(out_vertices, out_indices, triangles, 0.f, {1.f, 0.f, 0.f}); // red
        build_triangulated_face(out_vertices, out_indices, triangles, 10.f, {1.f, 0.f, 0.f}); // red
        break;

      case FaceType::WALL:
        std::println("Wall face found!");
        build_triangulated_face(out_vertices, out_indices, triangles, 0.f, {0.f, 1.f, 0.f}); // green
        build_triangulated_face(out_vertices, out_indices, triangles, 10.f, {0.f, 1.f, 0.f}); // green
        extrude_walls(out_vertices, out_indices, 10.f, contour);
        break;

      case FaceType::DOOR:
        std::println("Door face found!");
        build_triangulated_face(out_vertices, out_indices, triangles, 0.f, {0.f, 0.f, 1.f}); // blue
        build_triangulated_face(out_vertices, out_indices, triangles, 10.f, {0.f, 0.f, 1.f}); // blue
        break; 
        
      case FaceType::WINDOW:
        std::println("Window face found!");
        break; 

      default:
        break;
    }
    
    // auto outer_wall = std::vector<glm::dvec2>{};
    // extrude_walls(out_vertices, out_indices, ceil_H, contour, outer_wall);
  }
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