#include "reconstruction.hpp"

#include <GLFW/glfw3.h>

#include <print>
#include <format>
#include <algorithm>
#include <cstdlib>

#include <glm/ext/vector_float4.hpp>
#include <glm/glm.hpp> 
#include <stdexcept>

#include "geometry.hpp"
#include "dump.hpp"
#include "io/drw_parser.hpp"


// ============================
// Checkpoints 
// ============================

static bool checkpoint_raw_segments(const auto& walls, const auto& doors, const auto& windows)
{
  dump_segments_csv(walls, "walls_segments.csv");
  dump_segments_csv(doors, "doors_segments.csv");
  dump_segments_csv(windows, "windows_segments.csv");
  auto ret = std::system("python plot_segments.py");
  if (ret != 0) 
    throw std::runtime_error(std::format("[Checkpoint] attenzione: lo script `python plot_segments.py` è uscito con codice {}", ret));
  
  ret = std::system("xdg-open segments.png >/dev/null 2>&1 &");

  std::println("Continuare? [y/n] (default y):");
  auto answer = std::string{};
  std::getline(std::cin, answer);
  return answer.empty() || answer == "y" || answer == "Y";
}

static bool checkpoint_clusters(const auto& sample_points, const auto& clusters)
{
  dump_clusters_csv(sample_points, clusters, "clusters.csv");
  auto ret = std::system("python plot_clusters.py");
  if (ret != 0) 
    throw std::runtime_error(std::format("[Checkpoint] attenzione: lo script `python plot_clusters.py` è uscito con codice {}", ret));
    
  ret = std::system("xdg-open clusters.png >/dev/null 2>&1 &");
  std::println("Continuare? [y/n] (default y):");

  auto answer = std::string{};
  std::getline(std::cin, answer);
  return answer.empty() || answer == "y" || answer == "Y";
}

static bool checkpoint_faces(const auto& faces)
{
  dump_faces_csv(faces, "faces.csv");
  auto ret = std::system("python plot_faces.py");
  if (ret != 0) 
    throw std::runtime_error(std::format("[Checkpoint] attenzione: lo script `python plot_faces.py` è uscito con codice {}", ret));
    
  ret = std::system("xdg-open faces.png >/dev/null 2>&1 &");
  std::println("Continuare? [y/n] (default y):");

  auto answer = std::string{};
  std::getline(std::cin, answer);
  return answer.empty() || answer == "y" || answer == "Y";
}


// ============================
// Steps 
// ============================

static void primitives_extraction(ReconstructionContext& ctx, const auto& filename)
{
  auto parser = DRWParser{};
  auto dxf = dxfRW(filename.string().c_str());
  if (!dxf.read(&parser, false))
    throw std::runtime_error(std::format("Error reading DXF file (code: {}): {}", static_cast<i32>(dxf.getError()), filename.string()));

  ctx.walls = std::move(parser.walls);
  ctx.doors = std::move(parser.doors);
  ctx.windows = std::move(parser.windows);
  ctx.unit_scale = parser.unit_scale;
  auto house_bbox = calculate_bbox_2D(ctx.walls);
  if(ctx.unit_scale == 0.0)
  {
    std::println("Invalid unit scale. Trying to detect it based on the box area");
    ctx.unit_scale = detect_unit_scale(house_bbox.calculate_area());
  }
  
  std::println("Successfully parsed DXF file:\n unit scale: {} \n walls: {}\n door: {}\n windows: {}", 
    ctx.unit_scale,
    ctx.walls.size(), 
    ctx.doors.size(), 
    ctx.windows.size());
}

static void vertex_snapping(ReconstructionContext& ctx, f64 snap_eps)
{
  auto hash = SpatialHash{ snap_eps };
  auto edges = std::vector<Edge>{};
  auto wall_segments_view = std::array{ ctx.walls };
  for (const auto& seg : wall_segments_view | std::views::join)
  {
    auto v1 = hash.snap(seg.start);
    auto v2 = hash.snap(seg.end);
    if (v1 != v2)
      edges.push_back(Edge{ v1, v2, seg.layer });
  }

  auto& vertices = hash.vertices();
  std::println("Vertex snapping completed. Vertices: {}, edges: {}", vertices.size(), edges.size());

  ctx.hash = std::move(hash);
  ctx.edges = std::move(edges);
}

static void opening_reconstruction(ReconstructionContext& ctx,
                                   i32 num_samples,
                                   f64 eps)
{
  if(!ctx.doors.empty())
    doors_reconstruction(ctx.doors, ctx.hash, ctx.edges);

  if(!ctx.windows.empty())
  {
    auto sample_points = sample_segments(ctx.windows, num_samples);
    auto clusters = calculate_clusters(sample_points, eps);
    windows_reconstruction(sample_points, clusters, ctx.hash, ctx.edges);

    ctx.sample_points = std::move(sample_points);
    ctx.clusters = std::move(clusters);
  }
}

static void face_extraction(ReconstructionContext& ctx, auto& vertices, auto& edges)
{
  auto arrangement = build_arrangement(vertices, edges);
  std::println("Arrangement successfully completed: vertices={}, edges={}, faces={}", 
    arrangement.number_of_vertices(), 
    arrangement.number_of_edges(), 
    arrangement.number_of_faces());
  
  auto faces = extract_faces(arrangement);
  std::println("Extracted faces: {}", faces.size());

  ctx.arrangement = std::move(arrangement);
  ctx.faces = std::move(faces);
}

static ReconstructionResult build_mesh(auto& faces)
{
  auto vertices = std::vector<Vertex_PN>{};
  auto indices  = std::vector<u32>{};

  for(const auto& face : faces)
  {
    // Ensure CCW winding
    auto polyline = face.vertices;
    if (calculate_signed_area(polyline) < 0.0)
      std::ranges::reverse(polyline);
      
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

    constexpr auto CEIL_HEIGHT        = 1.0f * 8;
    constexpr auto DOOR_OFFSET        = 0.9f * 8;
    constexpr auto WINDOW_OFFSET_DOWN = 0.2f * 8;
    constexpr auto WINDOW_OFFSET_UP   = 0.7f * 8;
    switch(face.type)
    {
      case FaceType::FLOOR:
        std::println("FLOOR face found!");
        build_triangulated_face(vertices, indices, triangles, 0.f, { 1.f, 0.f, 0.f });
        // build_triangulated_face(out_vertices, out_indices, triangles, CEIL_HEIGHT, { 1.f, 0.f, 0.f });
        break;

      case FaceType::WALL:
        std::println("WALL face found!");
        // build_triangulated_face(out_vertices, out_indices, triangles, CEIL_HEIGHT, { 0.f, 1.f, 0.f });
        extrude_face(vertices, indices, 0, CEIL_HEIGHT, face);
        break;

      case FaceType::DOOR:
        std::println("DOOR face found!");
        // build_triangulated_face(out_vertices, out_indices, triangles, CEIL_HEIGHT, { 0.f, 0.f, 1.f });
        extrude_face(vertices, indices, DOOR_OFFSET, CEIL_HEIGHT, face);
        break;
        
      case FaceType::WINDOW:
        std::println("WINDOW face found!");
        build_triangulated_face(vertices, indices, triangles, WINDOW_OFFSET_DOWN, {1.f, 0.f, 1.f});
        build_triangulated_face(vertices, indices, triangles, WINDOW_OFFSET_UP, {1.f, 0.f, 1.f});
        // build_triangulated_face(out_vertices, out_indices, triangles, CEIL_HEIGHT, {1.f, 0.f, 1.f});
        extrude_face(vertices, indices, 0.0f, WINDOW_OFFSET_DOWN, face);
        extrude_face(vertices, indices, WINDOW_OFFSET_UP, CEIL_HEIGHT, face);
        break; 

      default:
        std::println("Unknown face type found!");
        break;
    }
  }

  auto mesh_data = ReconstructionResult{};
  mesh_data.mesh_vertices = std::move(vertices);
  mesh_data.mesh_indices = std::move(indices);
  return mesh_data;
}

ReconstructionResult reconstruction(struct GLFWwindow* window, const std::filesystem::path& filename)
{
  auto ctx = ReconstructionContext{};
  
  // parsing DXF model to extract primitives

  primitives_extraction(ctx, filename);
  auto& walls = ctx.walls;
  auto& doors = ctx.doors;
  auto& windows = ctx.windows;
  auto unit_scale = ctx.unit_scale;

  normalize_segments(unit_scale, walls);
  normalize_segments(unit_scale, doors);
  normalize_segments(unit_scale, windows);

  if(!checkpoint_raw_segments(walls, doors, windows))
    exit(0);
  
  // Vertex snapping with spatial hashing data structure: wall segments only

  vertex_snapping(ctx, 1e-4);
  auto& hash = ctx.hash;
  auto& edges = ctx.edges;
  auto& vertices = hash.vertices();

  // Topological reconstruction of openings

  opening_reconstruction(ctx, 10, 0.1);
  auto& sample_points = ctx.sample_points;
  auto& clusters = ctx.clusters;
  if(!sample_points.empty() && !checkpoint_clusters(sample_points, clusters))
    exit(0);

  // half hedge construction + face extraction
  
  face_extraction(ctx, vertices, edges);
  auto& faces = ctx.faces;

  if(!checkpoint_faces(faces))
    exit(0);

  // remove all FLOOR faces and push only one quad for floor

  std::erase_if(faces, [](auto face) { return face.type == FaceType::FLOOR; });
  auto house_bbox = calculate_bbox_2D(ctx.walls);
  auto floor_face = Face{};
  floor_face.vertices = {
    glm::dvec2(house_bbox.min.x, house_bbox.min.y),
    glm::dvec2(house_bbox.max.x, house_bbox.min.y),
    glm::dvec2(house_bbox.max.x, house_bbox.max.y),
    glm::dvec2(house_bbox.min.x, house_bbox.max.y) 
  };
  floor_face.type = FaceType::FLOOR;
  faces.push_back(std::move(floor_face));

  return build_mesh(faces);
}