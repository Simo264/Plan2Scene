#include <vector>
#include <filesystem>
#include <print>
#include <algorithm>

#include "geometry.hpp"
#include "arrangement.hpp"
#include "io/drw_parser.hpp"

#include "spatial_hashing.hpp"
#include "types.hpp"

#include "dbscan.h"

void reconstruction(const std::filesystem::path& filename,
                    std::vector<Vertex_PN>& out_vertices,
                    std::vector<u32>& out_indices)
{
  // parsing DXF model to extract primitives

  auto parser = DRWParser{};
  auto dxf = dxfRW(filename.string().c_str());
  if (!dxf.read(&parser, false))
    throw std::runtime_error(std::format("Error reading DXF file (code: {}): {}", static_cast<i32>(dxf.getError()), filename.string()));

  auto& walls = parser.walls;
  auto& doors = parser.doors;
  auto& windows = parser.windows;
  std::println("Successfully parsed DXF file:\n walls: {}\n door: {}\n windows: {}", walls.size(), doors.size(), windows.size());

  auto room_bbox = calculate_bbox_2D(walls);

  // detect the unit scale and normalize points
  
  if(parser.unit_scale == 0.0f)
    parser.unit_scale = detect_unit_scale(room_bbox.calculate_area());

  std::println("Unit scale: {}", parser.unit_scale);
  normalize_segments(parser.unit_scale, walls);
  normalize_segments(parser.unit_scale, doors);
  normalize_segments(parser.unit_scale, windows);

  // dump_segments(walls, "walls_segments.txt");
  // dump_segments(doors, "doors_segments.txt");
  // dump_segments(windows, "windows_segments.txt");
  // exit(0);
  
  // Vertex snapping with spatial hashing data structure: wall segments only

  auto hash = SpatialHash{ 1e-5 };
  auto edges = vertex_snapping(walls, hash);
  auto& vertices = hash.vertices();
  std::println("Vertex snapping completed. Vertices: {}, edges: {}", vertices.size(), edges.size());
  
  // Reconstructs door geometries

  doors_reconstruction(doors, hash, edges);

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

  auto arrangement = build_arrangement(vertices, edges);
  std::println("Arrangement successfully completed: vertices={}, edges={}, faces={}", 
    arrangement.number_of_vertices(), arrangement.number_of_edges(), arrangement.number_of_faces());

  // dump_faces(arrangement, "walls_segments.txt");
  // exit(0);
  
  // face extraction
  
  auto faces = extract_faces(arrangement);
  std::println("Extracted faces: {}", faces.size());
  std::erase_if(faces, [](auto face) { return face.type == FaceType::ROOM; });

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
    constexpr auto DOOR_OFFSET = 9.0f;
    switch(face.type)
    {
      case FaceType::ROOM:
        std::println("Room face found!");
        build_triangulated_face(out_vertices, out_indices, triangles, 0.f, { 1.f, 0.f, 0.f });         // red
        // build_triangulated_face(out_vertices, out_indices, triangles, CEIL_HEIGHT + 0.001f, { 1.f, 0.f, 0.f }); // red
        break;

      case FaceType::WALL:
        std::println("Wall face found!");
        //build_triangulated_face(out_vertices, out_indices, triangles, 0.f, { 0.f, 1.f, 0.f });         // green
        build_triangulated_face(out_vertices, out_indices, triangles, CEIL_HEIGHT, { 0.f, 1.f, 0.f }); // green
        extrude_face(out_vertices, out_indices, 0, CEIL_HEIGHT, face);
        break;

      case FaceType::DOOR:
        std::println("Door face found!");
        //build_triangulated_face(out_vertices, out_indices, triangles, 0.f, { 0.f, 0.f, 1.f });         // blue
        build_triangulated_face(out_vertices, out_indices, triangles, CEIL_HEIGHT, { 0.f, 0.f, 1.f }); // blue
        extrude_face(out_vertices, out_indices, DOOR_OFFSET, CEIL_HEIGHT, face);
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