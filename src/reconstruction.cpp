#include "reconstruction.hpp"

#include <GLFW/glfw3.h>

#include <format>
#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <vector>
#include <cstdio>

#include <glm/ext/vector_float4.hpp>
#include <glm/glm.hpp> 

#include "geometry.hpp"
#include "graphics/static_mesh.hpp"
#include "dump.hpp"
#include "globals.hpp"
#include "io/drw_parser.hpp"

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
  auto log_file = std::format("tmp/{}.log", script_name); // es. "plot_segments.py.log"
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
    if(std::filesystem::exists("tmp") == false)
      std::filesystem::create_directory("tmp");
    
    dump_segments_csv(walls, "tmp/walls_segments.csv");
    dump_segments_csv(doors, "tmp/doors_segments.csv");
    dump_segments_csv(windows, "tmp/windows_segments.csv");
    run_checkpoint_script("plot_segments.py");
  }

  void checkpoint_clusters(const std::vector<glm::dvec2>& sample_points,
                           const std::vector<std::vector<u32>>& clusters)
  {
    if(std::filesystem::exists("tmp") == false)
      std::filesystem::create_directory("tmp");

    dump_clusters_csv(sample_points, clusters, "tmp/clusters.csv");
    run_checkpoint_script("plot_clusters.py");
  }

  void checkpoint_faces(const std::vector<Face>& faces)
  {
    if(std::filesystem::exists("tmp") == false)
      std::filesystem::create_directory("tmp");

    dump_faces_csv(faces, "tmp/faces.csv");
    run_checkpoint_script("plot_faces.py");
  }

  // ============================
  // Steps 
  // ============================

  void primitives_extraction(ReconstructionContext& ctx, const std::filesystem::path& file)
  {
    auto parser = DRWParser{};
    auto dxf = dxfRW(file.string().c_str());
    if (!dxf.read(&parser, false))
      throw std::runtime_error(std::format("Error reading DXF file `{}` (code: {})", file.string(), static_cast<i32>(dxf.getError())));

    ctx.walls = std::move(parser.walls);
    ctx.doors = std::move(parser.doors);
    ctx.windows = std::move(parser.windows);
    ctx.unit_scale = g_config.unit_scale;

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
    triangulate_face(vertices, wall_indices, g_config.ceil_height, false, *floor_face);
    
    auto wall_faces = std::ranges::views::filter(faces, [](const Face& f) { return f.type == FaceType::WALL; });
    for(const auto& face : wall_faces)
    {
      extrude_face(vertices, wall_indices, 0.f, g_config.ceil_height, face);
    }

    auto door_faces = std::ranges::views::filter(faces, [](const Face& f) { return f.type == FaceType::DOOR; });
    for(const auto& face : door_faces)
    {
      extrude_face(vertices, wall_indices, g_config.door_frac_top, g_config.ceil_height, face);
      triangulate_face(vertices, wall_indices, g_config.door_frac_top, true, face);
    }

    auto window_faces = std::ranges::views::filter(faces, [](const Face& f) { return f.type == FaceType::WINDOW; });
    for(const auto& face : window_faces)
    {
      extrude_face(vertices, wall_indices, 0.f, g_config.window_frac_bottom, face);
      extrude_face(vertices, wall_indices, g_config.window_frac_top, g_config.ceil_height, face);
      triangulate_face(vertices, wall_indices, g_config.window_frac_bottom, true, face);
      triangulate_face(vertices, wall_indices, g_config.window_frac_top, false, face);
    }

    auto floor_range = PrimitiveRange{ 0, static_cast<u32>(floor_indices.size()), MaterialType::Floor };
    auto all_indices = std::vector<u32>{};
    all_indices.reserve(floor_indices.size() + wall_indices.size());
    all_indices.insert(all_indices.end(), floor_indices.begin(), floor_indices.end());

    auto wall_range = PrimitiveRange{ 
      static_cast<u32>(all_indices.size()), 
      static_cast<u32>(wall_indices.size()), 
      MaterialType::Wall };
    all_indices.insert(all_indices.end(), wall_indices.begin(), wall_indices.end());

    auto result = ReconstructionResult{};
    result.mesh_vertices = std::move(vertices);
    result.mesh_indices  = std::move(all_indices);
    result.primitives = { floor_range, wall_range };
    return result;
  }
}