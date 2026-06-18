#include <vector>
#include <filesystem>
#include <print>
#include <algorithm>

#include <glm/ext/vector_float4.hpp>
#include <glm/glm.hpp> 

#include "geometry.hpp"
#include "arrangement.hpp"
#include "io/drw_parser.hpp"

#include "spatial_hashing.hpp"
#include "types.hpp"
#include "utils.hpp"

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

  // dump_segments_csv(walls, "walls_segments.csv");
  // dump_segments_csv(doors, "doors_segments.csv");
  // dump_segments_csv(windows, "windows_segments.csv");
  // exit(0);
  
  // Vertex snapping with spatial hashing data structure: wall segments only

  auto hash = SpatialHash{ 1e-5 };
  auto edges = vertex_snapping(walls, hash);
  auto& vertices = hash.vertices();
  std::println("Vertex snapping completed. Vertices: {}, edges: {}", vertices.size(), edges.size());
  
  // Reconstructs door geometries

  doors_reconstruction(doors, hash, edges);

  // Group door and window segments into clusters using DBSCAN algorithm

  {
    constexpr auto num_samples = 10;
    auto points = std::vector<glm::dvec2>{};
    points.reserve(windows.size());
    for (const auto& w : windows) 
    {
      for (auto i = 0; i <= num_samples; ++i) 
      {
        auto t = static_cast<f64>(i) / num_samples;
        auto x = w.start.x + t * (w.end.x - w.start.x);
        auto y = w.start.y + t * (w.end.y - w.start.y);
        points.push_back(glm::dvec2{x, y});
      }
    }

    auto dbscan = DBSCAN<glm::dvec2, f64>(); 
    constexpr auto min_points = 2;
    constexpr auto tollerance = 1.0f;
    dbscan.Run(&points, 2, tollerance, min_points);

    auto clusters = dbscan.Clusters;
    for (auto i = 0ul; i < clusters.size(); ++i) 
    {
      const auto& cluster_indices = clusters[i];
      if (cluster_indices.empty()) 
        continue;
  
      auto box = calculate_bbox_2D(points, cluster_indices);      

      auto dx = box.max.x - box.min.x;
      auto dy = box.max.y - box.min.y;    
      glm::dvec2 p_start, p_end;

      // Identifichiamo il lato lungo ed estraiamo i punti mediani estremi
      if (dx > dy) 
      {
        // La finestra è prevalentemente orizzontale
        auto mid_y = (box.min.y + box.max.y) / 2.0; // Centro dello spessore
        p_start = glm::dvec2(box.min.x, mid_y);       // Estremo sinistro
        p_end   = glm::dvec2(box.max.x, mid_y);       // Estremo destro
      } 
      else 
      {
        // La finestra è prevalentemente verticale
        auto mid_x = (box.min.x + box.max.x) / 2.0; // Centro dello spessore
        p_start = glm::dvec2(mid_x, box.min.y);       // Estremo inferiore
        p_end   = glm::dvec2(mid_x, box.max.y);       // Estremo superiore
      }

      // 2. SNAPPING DEI PRIMI DUE VERTICI REALI (A e C)
      auto A_id = hash.find_nearest(p_start);
      auto C_id = hash.find_nearest(p_end);
      
      auto wall_dir = glm::normalize(vertices[C_id] - vertices[A_id]);
      
      auto nbrs_A = find_neighboors(A_id, edges);
      auto B_id = get_adjacent_vertex(wall_dir, A_id, nbrs_A, vertices);
      
      auto nbrs_C = find_neighboors(C_id, edges);
      auto D_id = get_adjacent_vertex(wall_dir, C_id, nbrs_C, vertices);
      
      auto P_A = vertices[A_id];
      auto P_C = vertices[C_id];
      auto P_B = vertices[B_id];
      auto P_D = vertices[D_id];
      
      auto window_close_left = B_id;
      auto window_close_right = D_id;
      
      auto proj_D = project_onto_segment(P_D, P_A, P_B);
      if (proj_D.is_inside) 
      {
        vertices.push_back(proj_D.point);
        VertexId B_prime_id = vertices.size() - 1;
        
        std::erase_if(edges, [&](const Edge& e) { return (e.v1 == A_id && e.v2 == B_id) || (e.v1 == B_id && e.v2 == A_id);  });
        
        edges.push_back(Edge{ A_id, B_prime_id, LayerType::WALL });
        edges.push_back(Edge{ B_prime_id, B_id, LayerType::WALL });
        window_close_left = B_prime_id;
      } 
      else 
      {
        auto proj_B = project_onto_segment(P_B, P_C, P_D);
        if (proj_B.is_inside) 
        {
          vertices.push_back(proj_B.point);
          VertexId D_prime_id = vertices.size() - 1;
          
          // Spezziamo l'originale bordo del muro C-D
          std::erase_if(edges, [&](const Edge& e) {  return (e.v1 == C_id && e.v2 == D_id) || (e.v1 == D_id && e.v2 == C_id); });
          
          edges.push_back(Edge{ C_id, D_prime_id, LayerType::WALL });
          edges.push_back(Edge{ D_prime_id, D_id, LayerType::WALL });
          window_close_right = D_prime_id;
        }
      }
      edges.push_back(Edge{ A_id, C_id, LayerType::WINDOW });
      edges.push_back(Edge{ window_close_left, window_close_right, LayerType::WINDOW });
    }
  }

  auto arrangement = build_arrangement(vertices, edges);
  std::println("Arrangement successfully completed: vertices={}, edges={}, faces={}", 
    arrangement.number_of_vertices(), arrangement.number_of_edges(), arrangement.number_of_faces());

  // dump_faces_csv(arrangement, "walls_segments.csv");
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
        build_triangulated_face(out_vertices, out_indices, triangles, 2.f, {1.f, 0.f, 1.f});  // purple
        build_triangulated_face(out_vertices, out_indices, triangles, 7.f, {1.f, 0.f, 1.f});  // purple
        // build_triangulated_face(out_vertices, out_indices, triangles, CEIL_HEIGHT, {1.f, 0.f, 1.f});  // purple
        extrude_face(out_vertices, out_indices, 0.0f, 2.0f, face);
        extrude_face(out_vertices, out_indices, 7.0f, CEIL_HEIGHT, face);
        break; 

      default:
        std::println("Unknown face type found!");
        break;
    }
  }
}