#include <algorithm>
#include <memory>
#include <print>
#include <filesystem>
#include <stdexcept>
#include <vector>

#include "types.hpp"
#include "utils.hpp"
#include "geometry.hpp"
#include "spatial_hashing.hpp"

#include "arrangement.hpp"
#include "io/drw_parser.hpp"
#include "io/gltf_exporter.hpp"
#include "graphics/mesh_visualizer.hpp"

#include <glm/trigonometric.hpp>
#include <glm/geometric.hpp>

void dump_raw_faces(const Arrangement& arr, std::string_view filename) 
{
  std::ofstream file(filename.data());
  if (!file.is_open()) return;

  file << std::fixed << std::setprecision(6);
  for (auto fit = arr.faces_begin(); fit != arr.faces_end(); ++fit) 
  {
      if (fit->is_unbounded() || !fit->has_outer_ccb()) continue;

      auto curr = fit->outer_ccb();
      auto first = curr;
      do {
          auto p1 = curr->source()->point();
          auto p2 = curr->target()->point();
          file << CGAL::to_double(p1.x()) << "," << CGAL::to_double(p1.y()) << "," << CGAL::to_double(p2.x()) << "," << CGAL::to_double(p2.y()) << "\n";
          ++curr;
      } while (curr != first);
  }
}


void parse_cad(const std::filesystem::path& filename,
               std::vector<Vertex_PN>& out_vertices,
               std::vector<u32>& out_indices)
{
  // parsing DXF model to extract segments

  auto parser = DRWParser{};
  auto dxf = dxfRW(filename.string().c_str());
  if (!dxf.read(&parser, false))
    throw std::runtime_error(std::format("Error reading DXF file (code: {}): {}", static_cast<i32>(dxf.getError()), filename.string()));

  auto& walls = parser.walls;
  auto& doors = parser.doors;
  auto& windows = parser.windows;
  std::println("Successfully parsed DXF file:\n walls: {}\n door: {}\n windows: {}", walls.size(), doors.size(), windows.size());

  auto room_bbox = calculate_bbox_2D(walls);

  // culling segments

  // std::erase_if(windows_segments, [&room_bbox](const Segment& door) { return !is_point_inside_bbox(room_bbox, door.p1); });
  // std::erase_if(doors, [&room_bbox](const Segment& door) { return !room_bbox.contains(door.start); });
  // std::println("After culling:\n door segments: {}\n window segments: {}", doors.size(), windows.size());

  // detect the unit scale and normalize
  
  if(parser.unit_scale == 0.0f)
    parser.unit_scale = detect_unit_scale(room_bbox.calculate_area());

  std::println("Unit scale: {}", parser.unit_scale);
  normalize_segments(parser.unit_scale, walls);
  normalize_segments(parser.unit_scale, doors);
  normalize_segments(parser.unit_scale, windows);

  dump_segments(walls, "walls_segments.txt");
  dump_segments(doors, "doors_segments.txt");
  dump_segments(windows, "windows_segments.txt");
  exit(0);
  
  // Vertex snapping with spatial hashing data structure: wall segments only

  auto hash = SpatialHash{ 1e-5 };
  auto edges = vertex_snapping(walls, hash);
  auto& vertices = hash.vertices();

  std::println("Vertex snapping completed. Vertices: {}, edges: {}", vertices.size(), edges.size());
  
#if 0
  // Reconstruct door segments by snapping their endpoints to the nearest vertices on the wall segments. 
  // This ensures that doors are properly connected to walls in the arrangement. 
  
  for(auto& door_segment : doors_segments)
  {
    auto A_id = hash.find_nearest(door_segment.p1);
    auto B_id = hash.find_nearest(door_segment.p2);
    auto wall_dir = glm::normalize(vertices[A_id] - vertices[B_id]);
    door_segment.p1 = vertices[A_id];
    door_segment.p2 = vertices[B_id];

    auto nbrs_A = find_neighboors(A_id, edges);
    auto A_prime_id = get_adjacent_vertex(wall_dir, A_id, nbrs_A, vertices);
    
    auto nbrs_B = find_neighboors(B_id, edges);
    auto B_prime_id = get_adjacent_vertex(wall_dir, B_id, nbrs_B, vertices);

    edges.push_back(Edge{ A_id, B_id, LayerType::DOOR });
    edges.push_back(Edge{ A_prime_id, B_prime_id, LayerType::DOOR });
  }
#endif

#if 0
  // Reconstruct window segments using bounding box of window segments and snapping to nearest vertices on wall segments.
 
  for(auto& window : windows_segments)
  {
    auto bbox = calculate_bbox_2D(window);
    
    auto p00 = glm::dvec2{ bbox.min.x, bbox.min.y };
    auto p01 = glm::dvec2{ bbox.min.x, bbox.max.y };
    auto p10 = glm::dvec2{ bbox.max.x, bbox.min.y };
    auto p11 = glm::dvec2{ bbox.max.x, bbox.max.y };
  
    auto A = hash.find_nearest(p00);
    auto B = hash.find_nearest(p01);
    auto C = hash.find_nearest(p10);
    auto D = hash.find_nearest(p11);
  
    edges.push_back(Edge{ A, B, LayerType::WINDOW });
    edges.push_back(Edge{ C, D, LayerType::WINDOW });
  }
#endif

  // Halfedge
  
  auto arrangement = build_arrangement(vertices, edges);
  std::println("Arrangement successfully completed: vertices={}, edges={}, faces={}", 
    arrangement.number_of_vertices(), arrangement.number_of_edges(), arrangement.number_of_faces());

  dump_raw_faces(arrangement, "walls_segments.txt");
  exit(0);
  
  // face extraction
  
  auto faces = extract_faces(arrangement);
  std::println("Extracted faces: {}", faces.size());
  
  room_bbox = calculate_bbox_2D(walls);
  auto floor_face = Face{};
  floor_face.vertices = {
    glm::dvec2(room_bbox.min.x, room_bbox.min.y),
    glm::dvec2(room_bbox.max.x, room_bbox.min.y),
    glm::dvec2(room_bbox.max.x, room_bbox.max.y),
    glm::dvec2(room_bbox.min.x, room_bbox.max.y) 
  };
  floor_face.edge_layers.assign(4, LayerType::NONE);
  floor_face.type = FaceType::ROOM;
  faces.push_back(std::move(floor_face));

  for(const auto& face : faces)
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

    constexpr auto CEIL_HEIGHT = 10.f;
    // constexpr auto DOOR_OFFSET = 9.0f;
    switch(face.type)
    {
      case FaceType::ROOM:
        std::println("Room face found!");
        build_triangulated_face(out_vertices, out_indices, triangles, 0.f, { 1.f, 0.f, 0.f });         // red
        //build_triangulated_face(out_vertices, out_indices, triangles, CEIL_HEIGHT + 0.001f, { 1.f, 0.f, 0.f }); // red
        break;

      case FaceType::WALL:
        std::println("Wall face found!");
        build_triangulated_face(out_vertices, out_indices, triangles, 0.f, { 0.f, 1.f, 0.f });         // green
        build_triangulated_face(out_vertices, out_indices, triangles, CEIL_HEIGHT, { 0.f, 1.f, 0.f }); // green
        extrude_face(out_vertices, out_indices, 0, CEIL_HEIGHT, face);
        break;

      case FaceType::DOOR:
        std::println("Door face found!");
        // build_triangulated_face(out_vertices, out_indices, triangles, 0.f, { 0.f, 0.f, 1.f });         // blue
        // build_triangulated_face(out_vertices, out_indices, triangles, CEIL_HEIGHT, { 0.f, 0.f, 1.f }); // blue
        // extrude_face(out_vertices, out_indices, DOOR_OFFSET, CEIL_HEIGHT, face);
        break; 
        
      case FaceType::WINDOW:
        std::println("Window face found!");
        // build_triangulated_face(out_vertices, out_indices, triangles, 0.f, {1.f, 0.f, 1.f});  // purple
        // build_triangulated_face(out_vertices, out_indices, triangles, 2.f, {1.f, 0.f, 1.f});  // purple
        // build_triangulated_face(out_vertices, out_indices, triangles, 7.f, {1.f, 0.f, 1.f});  // purple
        // build_triangulated_face(out_vertices, out_indices, triangles, CEIL_HEIGHT, {1.f, 0.f, 1.f});  // purple
        // extrude_face(out_vertices, out_indices, 0.0f, 2.0f, face);
        // extrude_face(out_vertices, out_indices, 7.0f, CEIL_HEIGHT, face);
        break; 

      default:
        std::println("Unknown face type found!");
        break;
    }
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
    throw std::runtime_error("Usage example:\n 1. /build/Plan2Scene --load <model/input.gltf>\n2. ./build/Plan2Scene --parse <cad/input.dxf>");

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

    // Center the vertices at the origin. No transform needed.
    center_mesh(vertices);
   
    // exporting mesh in GLTF
    // auto gltf_path = file_path.filename().replace_extension("gltf");
    // std::println("Model will be exported to: {}", gltf_path.string());
    // export_to_gltf(vertices, indices, gltf_path);
  }

  // --- visualize mesh ---
  // ----------------------
  auto visualizer = MeshVisualizer(1024, 768);
  visualizer.set_mesh(std::make_shared<StaticMesh>(
    vertices.data(), 
    vertices.size(),
    indices.data(),  
    indices.size()
  ));
  visualizer.camera().eye = { 0.f, 2.f, 10.f };
  visualizer.camera().set_orientation(glm::radians(glm::vec3{ -5.f, 0.f, 0.f }));
  visualizer.render();
  return 0;
}