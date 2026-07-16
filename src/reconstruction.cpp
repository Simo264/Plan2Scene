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
#include "log.hpp"
#include "types.hpp"

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
  auto log_file = std::format("out/{}.log", script_name); // es. "plot_segments.py.log"
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
    if(std::filesystem::exists("out") == false)
      std::filesystem::create_directory("out");
    
    dump_segments_csv(walls, "out/walls_segments.csv");
    dump_segments_csv(doors, "out/doors_segments.csv");
    dump_segments_csv(windows, "out/windows_segments.csv");
    run_checkpoint_script("plot_segments.py");
  }

  void checkpoint_clusters(const std::vector<glm::dvec2>& sample_points,
                           const std::vector<std::vector<u32>>& clusters)
  {
    if(std::filesystem::exists("out") == false)
      std::filesystem::create_directory("out");

    dump_clusters_csv(sample_points, clusters, "out/clusters.csv");
    run_checkpoint_script("plot_clusters.py");
  }

  void checkpoint_faces(const std::vector<Face>& faces)
  {
    if(std::filesystem::exists("out") == false)
      std::filesystem::create_directory("out");

    dump_faces_csv(faces, "out/faces.csv");
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

    auto bbox = BoundingBox2D(parser.walls);
    auto area = bbox.calculate_area();
    auto unit_scale = g_config.unit_scale;

    auto doors_before = parser.doors.size();
    parser.remove_duplicate_segments(parser.doors);
    auto doors_after = parser.doors.size();
    auto doors_removed = doors_before - doors_after;
    
    g_logger.push_message({std::format(
        "=== Segment Deduplication ===\n"
        " Doors before: {}\n"
        " Doors after:  {}\n"
        " Removed:      {} duplicates",
        doors_before, doors_after, doors_removed),
        LogLevel::Text});
    
    g_logger.push_message({std::format(
        "DXF file data:\n"
        " number of wall segments: {}\n"
        " number of door segments: {}\n"
        " number of window segments: {}\n"
        " area: {}\n"
        " unit scale: {}",
        parser.walls.size(), parser.doors.size(), parser.windows.size(), area, unit_scale),
        LogLevel::Text});
    
    normalize_segments(unit_scale, parser.walls);
    normalize_segments(unit_scale, parser.doors);
    normalize_segments(unit_scale, parser.windows);
    center_mesh(parser.walls, parser.doors, parser.windows);

    ctx.walls = std::move(parser.walls);
    ctx.doors = std::move(parser.doors);
    ctx.windows = std::move(parser.windows);
  }

  void vertex_snapping(ReconstructionContext& ctx, f64 snap_eps)
  {
    auto hash = SpatialHash{ snap_eps };
    auto edges = std::vector<Edge>{};
    for (const auto& seg : ctx.walls)
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
        doors_reconstruction(ctx.doors, ctx.hash, ctx.edges);
        g_logger.push_message({std::format("Processing {} door segments", ctx.doors.size()), LogLevel::Text});
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
        windows_reconstruction(ctx.sample_points, ctx.clusters, ctx.hash, ctx.edges);
        g_logger.push_message({std::format("Processing {} windows clusters", ctx.clusters.size()), 
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
    auto mesh_vertices       = std::vector<Vertex_PNT>{};
    auto mesh_floor_indices  = std::vector<u32>{};
    auto mesh_wall_indices   = std::vector<u32>{};
    auto result              = ReconstructionResult{};
    
    auto ceil_height         = g_config.ceil_height;
    auto door_height         = g_config.door_height;
    auto window_sill         = g_config.window_sill_height;
    auto window_height       = g_config.window_height;
    auto wall_tex_scaling    = g_config.wall_texture_scaling;
    auto floor_tex_scaling   = g_config.floor_texture_scaling;

    // =======================
    // Create floor plan
    // =======================
    auto floor_face = std::ranges::find_if(faces, [](const Face& f) { return f.type == FaceType::Floor; });
    floor_face->triangulate(mesh_vertices, mesh_floor_indices, 0.f, floor_tex_scaling, true);
    floor_face->triangulate(mesh_vertices, mesh_wall_indices, ceil_height, wall_tex_scaling, false);
    
    // =======================
    // Extrude walls
    // =======================
    auto wall_faces = std::ranges::views::filter(faces, [](const Face& f) { return f.type == FaceType::Wall; });
    for(const auto& face : wall_faces)
      face.extrude(mesh_vertices, mesh_wall_indices, 0.f, ceil_height, wall_tex_scaling);

    // =======================
    // Extrude doors
    // =======================
    auto door_faces = std::ranges::views::filter(faces, [](const Face& f) { return f.type == FaceType::Door; });
    g_logger.push_message({std::format("{} door faces found!", std::ranges::distance(door_faces)), LogLevel::Text});
    for(const auto& face : door_faces)
    {
      face.extrude(mesh_vertices, mesh_wall_indices, door_height, ceil_height, wall_tex_scaling);
      face.triangulate(mesh_vertices, mesh_wall_indices, door_height, wall_tex_scaling, true);

      auto opening = compute_opening_instance(face, OpeningType::Door, 0.0f, door_height);
      result.openings.push_back(opening);
    }

    // =======================
    // Extrude windows
    // =======================
    auto window_faces = std::ranges::views::filter(faces, [](const Face& f) { return f.type == FaceType::Window; });
    g_logger.push_message({std::format("{} door windows found!", std::ranges::distance(window_faces)), LogLevel::Text});
    for(const auto& face : window_faces)
    {
      face.extrude(mesh_vertices, mesh_wall_indices, 0.0f, window_sill, wall_tex_scaling);
      face.triangulate(mesh_vertices, mesh_wall_indices, window_sill, wall_tex_scaling, true);
      
      face.extrude(mesh_vertices, mesh_wall_indices, window_height, ceil_height, wall_tex_scaling);
      face.triangulate(mesh_vertices, mesh_wall_indices, window_height, wall_tex_scaling, false);

      auto opening = compute_opening_instance(face, OpeningType::Window, window_sill, window_height);
      result.openings.push_back(opening);
    }

    auto floor_range = PrimitiveRange{ 0, static_cast<u32>(mesh_floor_indices.size()), MaterialType::Floor };
    auto all_indices = std::vector<u32>{};
    all_indices.reserve(mesh_floor_indices.size() + mesh_wall_indices.size());
    all_indices.insert(all_indices.end(), mesh_floor_indices.begin(), mesh_floor_indices.end());

    auto wall_range = PrimitiveRange{ static_cast<u32>(all_indices.size()), static_cast<u32>(mesh_wall_indices.size()), MaterialType::Wall };
    all_indices.insert(all_indices.end(), mesh_wall_indices.begin(), mesh_wall_indices.end());
    
    result.mesh_vertices = std::move(mesh_vertices);
    result.mesh_indices  = std::move(all_indices);
    result.primitives = { floor_range, wall_range };
    return result;
  }
}