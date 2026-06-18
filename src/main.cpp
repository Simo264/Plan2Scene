#include <memory>

#include <filesystem>
#include <stdexcept>
#include <vector>
#include <print>

#include "types.hpp"
#include "geometry.hpp"

#include "io/gltf_exporter.hpp"
#include "graphics/mesh_visualizer.hpp"

#include <glm/trigonometric.hpp>
#include <glm/geometric.hpp>

void reconstruction(const std::filesystem::path& filename,
                    std::vector<Vertex_PN>& out_vertices,
                    std::vector<u32>& out_indices);

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
    reconstruction(file_path, vertices, indices);

    // Center the vertices at the origin. No transform needed.
    center_mesh(vertices);
   
    // exporting mesh in GLTF
    auto gltf_path = file_path.filename().replace_extension("gltf");
    std::println("Model will be exported to: {}", gltf_path.string());
    export_to_gltf(vertices, indices, gltf_path);
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