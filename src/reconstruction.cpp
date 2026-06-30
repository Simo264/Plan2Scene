#include "reconstruction.hpp"

#include <GLFW/glfw3.h>

#include <format>
#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <vector>
#include <print>
#include <cstdio>

#include <glm/ext/vector_float4.hpp>
#include <glm/glm.hpp> 

#include "geometry.hpp"
#include "graphics/static_mesh.hpp"
#include "log.hpp"
#include "dump.hpp"
#include "globals.hpp"
#include "io/drw_parser.hpp"

extern Logger g_logger;

ReconstructionStage next_stage(ReconstructionStage stage) 
{
  switch (stage) 
  {
    case ReconstructionStage::PrimitivesExtraction:  return ReconstructionStage::VertexSnapping;
    case ReconstructionStage::VertexSnapping:        return ReconstructionStage::ClustersExtraction;
    case ReconstructionStage::ClustersExtraction:    return ReconstructionStage::GapsReconstruction;
    case ReconstructionStage::GapsReconstruction:    return ReconstructionStage::FacesExtraction;
    case ReconstructionStage::FacesExtraction:       return ReconstructionStage::BuildMesh;
    case ReconstructionStage::BuildMesh:             return ReconstructionStage::RenderMesh;
    default:                                          return ReconstructionStage::None;
  }
}

bool stage_needs_confirmation(ReconstructionStage stage) 
{
  switch (stage) 
  {
      case ReconstructionStage::PrimitivesExtraction:
      case ReconstructionStage::ClustersExtraction:
      case ReconstructionStage::FacesExtraction:
        return true;
      default:
        return false;
  }
}

static void run_checkpoint_script(std::string_view script_name)
{
  auto log_file = std::format("{}.log", script_name); // es. "plot_segments.py.log"
  auto command = std::format("python {} > \"{}\" 2>&1", script_name, log_file);
  auto ret = std::system(command.c_str());
  if (ret != 0) 
  {
    auto output = std::string{};
    if (auto in = std::ifstream(log_file); in)
    {
      output = std::string(
        (std::istreambuf_iterator<char>(in)),
        std::istreambuf_iterator<char>()
      );
    }
    std::remove(log_file.c_str()); // delete log file

    throw std::runtime_error(std::format("Execution of `python {}` terminated with code {}.\n{}",script_name, ret, output));
  }
}

namespace Reconstruction
{
  // ============================
  // Checkpoints 
  // ============================

  void checkpoint_raw_segments(const std::vector<Segment>& walls, 
                               const std::vector<Segment>& doors, 
                               const std::vector<Segment>& windows)
  {
    dump_segments_csv(walls, "walls_segments.csv");
    dump_segments_csv(doors, "doors_segments.csv");
    dump_segments_csv(windows, "windows_segments.csv");
    run_checkpoint_script("plot_segments.py");
  }

  void checkpoint_clusters(const std::vector<glm::dvec2>& sample_points,
                           const std::vector<std::vector<u32>>& clusters)
  {
    dump_clusters_csv(sample_points, clusters, "clusters.csv");
    run_checkpoint_script("plot_clusters.py");
  }

  void checkpoint_faces(const std::vector<Face>& faces)
  {
    dump_faces_csv(faces, "faces.csv");
    run_checkpoint_script("plot_faces.py");
  }

  // ============================
  // Steps 
  // ============================

  void primitives_extraction(ReconstructionContext& ctx, const std::filesystem::path& filename)
  {
    auto parser = DRWParser{};
    auto dxf = dxfRW(filename.string().c_str());
    if (!dxf.read(&parser, false))
      throw std::runtime_error(std::format("Error reading DXF file `{}` (code: {})", filename.string(), static_cast<i32>(dxf.getError())));

    ctx.walls = std::move(parser.walls);
    ctx.doors = std::move(parser.doors);
    ctx.windows = std::move(parser.windows);
    ctx.unit_scale = unit_scale;
    
    // auto house_bbox = calculate_bbox_2D(ctx.walls);
    // if(ctx.unit_scale == 0.0)
    // {
    //   g_logger.push_message({"Invalid unit scale. Trying to detect it based on the box area", LogLevel::Warning});
    //   ctx.unit_scale = detect_unit_scale(house_bbox.calculate_area());
    // }

    g_logger.push_message({std::format(
        "DXF file data:\n unit scale: {} \n number of wall segments: {}\n number of door segments: {}\n number of window segments: {}",
        ctx.unit_scale, ctx.walls.size(), ctx.doors.size(), ctx.windows.size()),
        LogLevel::Text});
    
    normalize_segments(ctx.unit_scale, ctx.walls);
    normalize_segments(ctx.unit_scale, ctx.doors);
    normalize_segments(ctx.unit_scale, ctx.windows);
  }

  void vertex_snapping(ReconstructionContext& ctx, f64 snap_eps)
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
    ctx.hash = std::move(hash);
    ctx.edges = std::move(edges);
  }

  void clusters_extraction(ReconstructionContext& ctx, i32 num_samples, f64 eps)
  {
    if(!ctx.windows.empty())
    {
      auto sample_points = sample_segments(ctx.windows, num_samples);
      auto clusters = calculate_clusters(sample_points, eps);
      ctx.sample_points = std::move(sample_points);
      ctx.clusters = std::move(clusters);
    }
  }

  void gaps_reconstruction(ReconstructionContext& ctx)
  {
    if(!ctx.doors.empty())
    {
      try 
      {
        auto edges_before_doors = ctx.edges.size();
        doors_reconstruction(ctx.doors, ctx.hash, ctx.edges);
        auto doors_edges_added = ctx.edges.size() - edges_before_doors;
        g_logger.push_message({std::format("Doors: {} processed -> {} edges added", ctx.doors.size(), doors_edges_added), 
          LogLevel::Text});
      } 
      catch (const std::exception& e) 
      {
        throw std::runtime_error(std::format("Door processing failed.\n{}", e.what()));
      }
    }
    
    if(!ctx.sample_points.empty() && !ctx.clusters.empty())
    {
      try
      {
        auto edges_before_windows = ctx.edges.size();
        windows_reconstruction(ctx.sample_points, ctx.clusters, ctx.hash, ctx.edges);
        auto windows_edges_added = ctx.edges.size() - edges_before_windows;
        g_logger.push_message({std::format("Windows: {} clusters processed -> {} edges added", ctx.clusters.size(), windows_edges_added), 
          LogLevel::Text});
      } 
      catch (const std::exception& e) 
      {
        throw std::runtime_error(std::format("Window processing failed.\n{}", e.what()));
      }
    }
  }

  void faces_extraction(ReconstructionContext& ctx, 
                       const std::vector<glm::dvec2>& vertices, 
                       const std::vector<Edge>& edges)
  {
    auto arrangement = build_arrangement(vertices, edges);
    auto faces = extract_faces(arrangement);
  
    ctx.arrangement = std::move(arrangement);
    ctx.faces = std::move(faces);
  }

  ReconstructionResult build_mesh(const std::vector<Face>& faces)
  {
    auto vertices = std::vector<Vertex_PNT>{};
    auto floor_indices  = std::vector<u32>{};
    auto wall_indices   = std::vector<u32>{};

    auto floor_face = std::ranges::find_if(faces, [](const Face& f) { return f.type == FaceType::FLOOR; });
    triangulate_face(vertices, floor_indices, 0.f, true, *floor_face);
    
    auto wall_faces = std::ranges::views::filter(faces, [](const Face& f) { return f.type == FaceType::WALL; });
    for(const auto& face : wall_faces)
      extrude_face(vertices, wall_indices, 0.f, ceil_height_meters, face);

    auto door_faces = std::ranges::views::filter(faces, [](const Face& f) { return f.type == FaceType::DOOR; });
    for(const auto& face : door_faces)
      extrude_face(vertices, wall_indices, door_frac_top, ceil_height_meters, face);

    // auto window_faces = std::ranges::views::filter(faces, [](const Face& f) { return f.type == FaceType::WINDOW; });
    // for(const auto& face : window_faces)
    // {
    //   extrude_face(vertices, wall_indices, 0.f, WINDOW_FRAC_BOTTOM, face);
    //   extrude_face(vertices, wall_indices, WINDOW_FRAC_TOP, CEIL_HEIGHT, face);
    // }
    
#if 0 
    for(const auto& face : faces)
    {
      auto face_bbox = calculate_bbox_2D(face);
      
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

      switch(face.type)
      {
        case FaceType::FLOOR:
          std::println("FLOOR face found!");
          build_triangulated_face(vertices, floor_indices, triangles, 0.f, true, face_bbox);
          // build_triangulated_face(vertices, indices, triangles, CEIL_HEIGHT, false);
          break;

        case FaceType::WALL:
          std::println("WALL face found!");
          build_triangulated_face(vertices, wall_indices, triangles, CEIL_HEIGHT, true, face_bbox);
          extrude_face(vertices, wall_indices, 0, CEIL_HEIGHT, face);
          break;

        case FaceType::DOOR:
          std::println("DOOR face found!");
          build_triangulated_face(vertices, wall_indices, triangles, CEIL_HEIGHT, true, face_bbox);
          extrude_face(vertices, wall_indices, DOOR_TOP, CEIL_HEIGHT, face);
          break;
          
        case FaceType::WINDOW:
          std::println("WINDOW face found!");
          build_triangulated_face(vertices, wall_indices, triangles, WINDOW_BOTTOM, true, face_bbox);
          build_triangulated_face(vertices, wall_indices, triangles, WINDOW_TOP, false, face_bbox);
          build_triangulated_face(vertices, wall_indices, triangles, CEIL_HEIGHT, true, face_bbox);
          extrude_face(vertices, wall_indices, 0.0f, WINDOW_BOTTOM, face);
          extrude_face(vertices, wall_indices, WINDOW_TOP, CEIL_HEIGHT, face);
          break; 

        default:
          std::println("Unknown face type found!");
          break;
      }
    }
#endif

    auto all_indices = std::vector<u32>{};
    all_indices.reserve(floor_indices.size() + wall_indices.size());

    auto floor_range = PrimitiveRange{ 
      .index_offset=0, 
      .index_count=u32(floor_indices.size()),
      .material=MaterialType::Floor 
    };
    auto wall_range = PrimitiveRange{ 
      .index_offset=u32(all_indices.size()), 
      .index_count=u32(wall_indices.size()), 
      .material=MaterialType::Wall 
    };
    all_indices.insert(all_indices.end(), floor_indices.begin(), floor_indices.end());
    all_indices.insert(all_indices.end(), wall_indices.begin(), wall_indices.end());
    
    auto result = ReconstructionResult{};
    result.mesh_vertices = std::move(vertices);
    result.mesh_indices  = std::move(all_indices);
    result.primitives = { floor_range, wall_range };
    return result;
  }
}