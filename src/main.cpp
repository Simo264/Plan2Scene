#include <filesystem>
#include <stdexcept>
#include <vector>
#include <memory>
#include <print>

#include "types.hpp"
#include "geometry.hpp"
#include "reconstruction.hpp"

#include "io/gltf_exporter.hpp"

#include "gui/gui.hpp"

#include <glm/trigonometric.hpp>
#include <glm/geometric.hpp>

int main(int argc, char* argv[])
{
  if(argc != 2)
    throw std::runtime_error("Usage: ./build/Plan2Scene <input.dxf>");
  
  auto file_path = std::filesystem::path(argv[1]);
  if(!std::filesystem::exists(file_path))
    throw std::runtime_error(std::format("Input file not found: {}", file_path.string()));

  auto window_context = init_window_context(1024, 768);
  // auto rec_result = reconstruction(window_context, file_path);
  // auto& vertices = rec_result.mesh_vertices;
  // auto& indices = rec_result.mesh_indices;
  
  while (!glfwWindowShouldClose(window_context))
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear buffers to preset values  

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    setup_docking();

    viewport_panel();
    properties_panel();
    log_panel();
     
    render_gui(); 

    glfwSwapBuffers(window_context);
    glfwPollEvents();
  }
  glfwTerminate();
  


  // Center the vertices at the origin. No transform needed.
  // center_mesh(vertices);
  
  // exporting mesh in GLTF
  // auto gltf_path = file_path.filename().replace_extension("gltf");
  // export_to_gltf(vertices, indices, gltf_path);
  // std::println("Model exported successfully: {}", gltf_path.string());

  // --- visualize mesh ---
  // ----------------------
  // auto visualizer = MeshVisualizer(1024, 768);
  // visualizer.set_mesh(std::make_shared<StaticMesh>(
  //   vertices.data(), 
  //   vertices.size(),
  //   indices.data(),  
  //   indices.size()
  // ));
  // visualizer.camera().eye = { 0.f, 2.f, 10.f };
  // visualizer.camera().set_orientation(glm::radians(glm::vec3{ -5.f, 0.f, 0.f }));
  // visualizer.render();
  return 0;
}