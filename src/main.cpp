#include <memory>

#include <filesystem>
#include <stdexcept>
#include <vector>
#include <print>

#include "types.hpp"
#include "geometry.hpp"
#include "reconstruction.hpp"

#include "io/gltf_exporter.hpp"
#include "graphics/mesh_visualizer.hpp"

#include <glm/trigonometric.hpp>
#include <glm/geometric.hpp>


int main(int argc, char* argv[])
{
  if(argc != 2)
    throw std::runtime_error("Usage: /build/Plan2Scene <cad/input.dxf>");
  
  auto file_path = std::filesystem::path(argv[1]);
  if(!std::filesystem::exists(file_path))
    throw std::runtime_error(std::format("Input file not found: {}", file_path.string()));
  
  auto rec_result = reconstruction(file_path);
  auto& vertices = rec_result.mesh_vertices;
  auto& indices = rec_result.mesh_indices;

  // Center the vertices at the origin. No transform needed.
  center_mesh(vertices);
  
  // exporting mesh in GLTF
  auto gltf_path = file_path.filename().replace_extension("gltf");
  std::println("Model will be exported to: {}", gltf_path.string());
  export_to_gltf(vertices, indices, gltf_path);

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