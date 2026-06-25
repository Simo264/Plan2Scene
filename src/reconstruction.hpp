#pragma once

#include "types.hpp"
#include "spatial_hashing.hpp"
#include "arrangement.hpp"

#include <vector>
#include <filesystem>

struct ReconstructionResult
{
  std::vector<Vertex_PN> mesh_vertices;
  std::vector<u32> mesh_indices;
};

struct ReconstructionContext
{
  std::vector<Segment> walls, doors, windows;
  f64 unit_scale;

  SpatialHash hash;
  std::vector<Edge> edges;

  std::vector<glm::dvec2> sample_points;
  std::vector<std::vector<u32>> clusters;

  Arrangement arrangement;
  std::vector<Face> faces;
};

enum class ReconstructionStage
{
  PrimitiveExtraction,
  OpeningReconstruction,
  FaceExtraction,
  BuildMesh,
  RenderMesh,
  None,
};

ReconstructionStage next_stage(ReconstructionStage p);

namespace Reconstruction
{
  void primitives_extraction_normalization(ReconstructionContext& ctx, 
                                           const std::filesystem::path& filename);
  void checkpoint_raw_segments(const std::vector<Segment>& walls, 
                               const std::vector<Segment>& doors, 
                               const std::vector<Segment>& windows);

  void vertex_snapping(ReconstructionContext& ctx, f64 snap_eps);
  void opening_reconstruction(ReconstructionContext& ctx, i32 num_samples, f64 eps);
  void checkpoint_clusters(const std::vector<glm::dvec2>& sample_points, 
                           const std::vector<std::vector<u32>>& clusters);

  void face_extraction(ReconstructionContext& ctx,
                       const std::vector<glm::dvec2>& vertices, 
                       const std::vector<Edge>& edges);
  void checkpoint_faces(const std::vector<Face>& faces);

  ReconstructionResult build_mesh(const std::vector<Face>& faces);
};