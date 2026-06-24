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


ReconstructionResult reconstruction(struct GLFWwindow* window, const std::filesystem::path& filename);
