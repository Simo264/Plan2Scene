#include <filesystem>
#include <shared_mutex>
#include <stdexcept>
#include <vector>
#include <memory>
#include <print>
#include <atomic>

#include "imgui.h"
#include "types.hpp"
#include "geometry.hpp"
#include "reconstruction.hpp"

#include "io/gltf_exporter.hpp"
#include "graphics/texture.hpp"

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
  
  auto viewport_image = Texture{};
  

  enum class Phase
  {
    PrimitiveExtraction,
    OpeningReconstruction,
    FaceExtraction,
    BuildMesh,
    None,
  };
  enum class PhaseState { Idle, Running };

  auto current_phase = Phase::PrimitiveExtraction;
  auto current_phase_state = std::atomic<PhaseState>{ PhaseState::Idle };
  
  auto phase_start_time = std::chrono::steady_clock::now();
  auto worker = std::optional<std::jthread>{};
  auto worker_is_done = std::atomic<bool>{ false };
  auto build_result = ReconstructionResult{};
  auto ctx = ReconstructionContext{};

  while (!glfwWindowShouldClose(window_context))
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear buffers to preset values

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    setup_docking();

    // =======================================================
    // Phase 1: primitive extraction and normalize segments
    // =======================================================

    if(current_phase == Phase::PrimitiveExtraction && current_phase_state == PhaseState::Idle)
    {
      auto elapsed = std::chrono::steady_clock::now() - phase_start_time;
      if (elapsed >= std::chrono::seconds(2)) 
      {
        current_phase_state = PhaseState::Running;
        worker.emplace([&] {
          Reconstruction::primitives_extraction_normalization(ctx, file_path);
          Reconstruction::checkpoint_raw_segments(ctx.walls, ctx.doors, ctx.windows);
          worker_is_done = true;
          phase_start_time = std::chrono::steady_clock::now();
        });
      }
    }

    // =======================================================
    // Phase 2: vertex snapping and opening reconstruction
    // =======================================================


    else if(current_phase == Phase::OpeningReconstruction && current_phase_state == PhaseState::Idle)
    {
      auto elapsed = std::chrono::steady_clock::now() - phase_start_time;
      if (elapsed >= std::chrono::seconds(2))
      {
        current_phase_state = PhaseState::Running;
        worker.emplace([&] {
          Reconstruction::vertex_snapping(ctx, 1e-4);
          Reconstruction::opening_reconstruction(ctx, 10, 0.1);
          Reconstruction::checkpoint_clusters(ctx.sample_points, ctx.clusters);
          worker_is_done = true;
          phase_start_time = std::chrono::steady_clock::now();
        });
      }
    }

    // =======================================================
    // Phase 3: faces extraction
    // =======================================================

    else if(current_phase == Phase::FaceExtraction && current_phase_state == PhaseState::Idle)
    {
      auto elapsed = std::chrono::steady_clock::now() - phase_start_time;
      if (elapsed >= std::chrono::seconds(2))
      {
        current_phase_state = PhaseState::Running;
        worker.emplace([&] {
          Reconstruction::face_extraction(ctx, ctx.hash.vertices(), ctx.edges);
          Reconstruction::checkpoint_faces(ctx.faces);
          worker_is_done = true;
          phase_start_time = std::chrono::steady_clock::now();
        });
      }
    }

    // =======================================================
    // Phase 4: mesh building
    // =======================================================

    else if(current_phase == Phase::BuildMesh && current_phase_state == PhaseState::Idle)
    {
      auto elapsed = std::chrono::steady_clock::now() - phase_start_time;
      if (elapsed >= std::chrono::seconds(2))
      {
        current_phase_state = PhaseState::Running;
        worker.emplace([&] {
          // remove all FLOOR faces and push only one quad for floor

          // std::erase_if(ctx.faces, [](auto face) { return face.type == FaceType::FLOOR; });
          // auto house_bbox = calculate_bbox_2D(ctx.walls);
          // auto floor_face = Face{};
          // floor_face.vertices = {
          //   glm::dvec2(house_bbox.min.x, house_bbox.min.y),
          //   glm::dvec2(house_bbox.max.x, house_bbox.min.y),
          //   glm::dvec2(house_bbox.max.x, house_bbox.max.y),
          //   glm::dvec2(house_bbox.min.x, house_bbox.max.y) 
          // };
          // floor_face.type = FaceType::FLOOR;
          // ctx.faces.push_back(std::move(floor_face));
          
          build_result = Reconstruction::build_mesh(ctx.faces);
          worker_is_done = true;
          phase_start_time = std::chrono::steady_clock::now();
        });
      }
    }



    if (current_phase == Phase::PrimitiveExtraction && current_phase_state == PhaseState::Running && worker_is_done.load())
    {
      current_phase_state = PhaseState::Idle;
      current_phase = Phase::OpeningReconstruction;
      worker_is_done = false;
      if(std::filesystem::exists("segments.png"))
      {
        auto plot_image = Texture::create_from_file("segments.png");
        if(viewport_image.is_valid())
          viewport_image.destroy();
        viewport_image = plot_image;
      }
    }
    else if (current_phase == Phase::OpeningReconstruction && current_phase_state == PhaseState::Running && worker_is_done.load())
    {
      current_phase_state = PhaseState::Idle;
      current_phase = Phase::FaceExtraction;
      worker_is_done = false;
      if(std::filesystem::exists("clusters.png"))
      {
        auto plot_image = Texture::create_from_file("clusters.png");
        if(viewport_image.is_valid())
          viewport_image.destroy();
        viewport_image = plot_image;
      }
    }
    else if (current_phase == Phase::FaceExtraction && current_phase_state == PhaseState::Running && worker_is_done.load())
    {
      current_phase_state = PhaseState::Idle;
      current_phase = Phase::BuildMesh;
      worker_is_done = false;
      if(std::filesystem::exists("faces.png"))
      {
        auto plot_image = Texture::create_from_file("faces.png");
        if(viewport_image.is_valid())
          viewport_image.destroy();
        viewport_image = plot_image;
      }
    }
    else if (current_phase == Phase::BuildMesh && current_phase_state == PhaseState::Running && worker_is_done.load())
    {
      current_phase_state = PhaseState::Idle;
      current_phase = Phase::None;
      worker_is_done = false;

      auto elapsed = std::chrono::steady_clock::now() - phase_start_time;
      if (elapsed >= std::chrono::seconds(2))
      {
        exit(0);
      }
    }



    if (ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoCollapse))
    {
      ImVec2 window_size = ImGui::GetContentRegionAvail(); 
      if (viewport_image.is_valid()) 
        ImGui::Image(viewport_image.id(), window_size);
    }
    ImGui::End();

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