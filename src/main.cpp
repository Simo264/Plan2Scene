#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <filesystem>
#include <stdexcept>
#include <atomic>
#include <print>

#include "imgui.h"
#include "types.hpp"
#include "geometry.hpp"
#include "reconstruction.hpp"
#include "graphics/texture.hpp"
#include "graphics/pipeline.hpp"
#include "graphics/camera.hpp"
#include "graphics/static_mesh.hpp"
#include "graphics/transformation.hpp"
#include "graphics/framebuffer.hpp"

#include "io/gltf_exporter.hpp"
#include "gui/gui.hpp"

#include <glm/trigonometric.hpp>
#include <glm/geometric.hpp>

static void create_gl_pipeline_object(ShaderProgram& vertex_program, ShaderProgram& fragment_program, ProgramPipelineObject& pipeline)
{
  auto shaders_dir = std::filesystem::current_path() / "shaders";

  auto vertex_shader_obj = ShaderObject{};
  vertex_shader_obj.create(ShaderStage::Vertex);
  vertex_shader_obj.load_source_code(shaders_dir / "basic_shader.vert.glsl");
  vertex_shader_obj.compile();
  auto status = vertex_shader_obj.check_compile_status();
  if (!status)
    std::println("Shader compilation error: {}", vertex_shader_obj.get_compile_log());

  auto fragment_shader_obj = ShaderObject{};
  fragment_shader_obj.create(ShaderStage::Fragment);
  fragment_shader_obj.load_source_code(shaders_dir / "basic_shader.frag.glsl");
  fragment_shader_obj.compile();
  status = fragment_shader_obj.check_compile_status();
  if (!status)
    std::println("Shader compilation error: {}", fragment_shader_obj.get_compile_log());

  vertex_program.create();
  vertex_program.attach_shader(vertex_shader_obj);
  vertex_program.set_separable(true);
  vertex_program.link();
  status = vertex_program.check_link_status();
  if (!status)
    std::println("Link status: {}", vertex_program.get_link_log());

  vertex_program.detach_shader(vertex_shader_obj);

  fragment_program.create();
  fragment_program.attach_shader(fragment_shader_obj);
  fragment_program.set_separable(true);
  fragment_program.link();
  status = fragment_program.check_link_status();
  if (!status)
    std::println("Link status: {}", fragment_program.get_link_log());

  fragment_program.detach_shader(fragment_shader_obj);

  pipeline.create();
  pipeline.bind_program_stage(PipelineStage::VertexShader, vertex_program);
  pipeline.bind_program_stage(PipelineStage::FragmentShader, fragment_program);
  status = pipeline.validate_pipeline();
  if (!status)
    std::println("pipeline object status: {}", pipeline.get_validation_status());
}

static void handle_camera_input(GLFWwindow* window, Camera& camera)
{
  constexpr auto velocity = 0.1f;
  
  if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    camera.rotate_pitch(+glm::radians(1.0f));
  if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  camera.rotate_pitch(-glm::radians(1.0f));
  if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  camera.rotate_yaw(+glm::radians(1.0f));
  if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) camera.rotate_yaw(-glm::radians(1.0f));
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)     camera.eye += camera.gaze() * velocity;
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)     camera.eye -= camera.gaze() * velocity;
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)     camera.eye -= camera.right() * velocity;
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)     camera.eye += camera.right() * velocity;
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) camera.eye += camera.up() * velocity;
  if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) camera.eye -= camera.up() * velocity;
}

static void create_framebuffer(FrameBuffer& fb, Texture& color, Texture& depth, i32 width, i32 height)
{
  color = Texture::create_empty(width, height, ...);
  depth = Texture::create_empty(width, height, ...);

  fb.create();
  fb.attach_texture(..., color, 0);
  fb.attach_texture(..., depth, 0);

  if (!fb.check_status())
    throw std::runtime_error("Error on create Framebuffer object");
}


int main(int argc, char* argv[])
{
  if(argc != 2)
    throw std::runtime_error("Usage: ./build/Plan2Scene <input.dxf>");
  
  auto file_path = std::filesystem::path(argv[1]);
  if(!std::filesystem::exists(file_path))
    throw std::runtime_error(std::format("Input file not found: {}", file_path.string()));

  auto viewport_info = ViewportInfo{};
  viewport_info.width = 1024;
  viewport_info.height = 768;
  viewport_info.aspect = static_cast<f32>(viewport_info.width) / static_cast<f32>(viewport_info.height);

  auto window_context = init_window_context(viewport_info.width, viewport_info.height);
 
  auto viewport_framebuffer = FrameBuffer{};
  auto fb_color_texture = Texture{};
  auto fb_depth_texture = Texture{};
  create_framebuffer(viewport_framebuffer, fb_color_texture, fb_depth_texture, static_cast<i32>(viewport_info.width), static_cast<i32>(viewport_info.height));
  
  ShaderProgram vertex_program, fragment_program;
  ProgramPipelineObject pipeline;
  create_gl_pipeline_object(vertex_program, fragment_program, pipeline);
  auto camera = Camera(0.1f, 100.0f, 45.f, viewport_info.aspect);
  
  auto current_stage = ReconstructionStage::PrimitiveExtraction;
  auto worker_state = std::atomic<ThreadState>{ ThreadState::Idle };
  
  auto worker = std::optional<std::jthread>{};
  auto worker_is_done = std::atomic<bool>{ false };
  auto build_result = ReconstructionResult{};
  auto ctx = ReconstructionContext{};

  auto static_mesh = std::shared_ptr<StaticMesh>{};
  auto mesh_transform = Transformation{};

  auto viewport_image = Texture{};
  while (!glfwWindowShouldClose(window_context))
  {
    glfwPollEvents();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear buffers to preset values
    if (glfwGetKey(window_context, GLFW_KEY_ESCAPE) == GLFW_PRESS) 
      glfwSetWindowShouldClose(window_context, GLFW_TRUE);

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    setup_docking();

    // =======================================================
    // Phase 1: primitive extraction and normalize segments
    // =======================================================

    if(current_stage == ReconstructionStage::PrimitiveExtraction && worker_state == ThreadState::Idle)
    {
      worker_state = ThreadState::Running;
      worker.emplace([&] {
        Reconstruction::primitives_extraction_normalization(ctx, file_path);
        Reconstruction::checkpoint_raw_segments(ctx.walls, ctx.doors, ctx.windows);
        worker_is_done = true;
      });
    }

    // =======================================================
    // Phase 2: vertex snapping and opening reconstruction
    // =======================================================

    else if(current_stage == ReconstructionStage::OpeningReconstruction && worker_state == ThreadState::Idle)
    {
      worker_state = ThreadState::Running;
      worker.emplace([&] {
        Reconstruction::vertex_snapping(ctx, 1e-4);
        Reconstruction::opening_reconstruction(ctx, 10, 0.1);
        Reconstruction::checkpoint_clusters(ctx.sample_points, ctx.clusters);
        worker_is_done = true;
      });
    }

    // =======================================================
    // Phase 3: faces extraction
    // =======================================================

    else if(current_stage == ReconstructionStage::FaceExtraction && worker_state == ThreadState::Idle)
    {
      worker_state = ThreadState::Running;
      worker.emplace([&] {
        Reconstruction::face_extraction(ctx, ctx.hash.vertices(), ctx.edges);
        Reconstruction::checkpoint_faces(ctx.faces);
        worker_is_done = true;
      });
    }

    // =======================================================
    // Phase 4: mesh building
    // =======================================================

    else if(current_stage == ReconstructionStage::BuildMesh && worker_state == ThreadState::Idle)
    {
      worker_state = ThreadState::Running;
      worker.emplace([&] {
        // remove all FLOOR faces and push only one quad for floor

        std::erase_if(ctx.faces, [](auto face) { return face.type == FaceType::FLOOR; });
        
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
      });
    }

    // =======================================================
    // load plots and update viewport image texture
    // =======================================================

    if (current_stage == ReconstructionStage::PrimitiveExtraction && worker_state == ThreadState::Running && worker_is_done.load())
    {
      worker_state = ThreadState::WaitingConfirmation;
      worker_is_done = false;
      if(std::filesystem::exists("segments.png"))
      {
        auto plot_image = Texture::create_from_file("segments.png");
        if(viewport_image.is_valid())
          viewport_image.destroy();
        viewport_image = plot_image;
      }
    }
    else if (current_stage == ReconstructionStage::OpeningReconstruction && worker_state == ThreadState::Running && worker_is_done.load())
    {
      worker_state = ThreadState::WaitingConfirmation;
      worker_is_done = false;
      if(std::filesystem::exists("clusters.png"))
      {
        auto plot_image = Texture::create_from_file("clusters.png");
        if(viewport_image.is_valid())
          viewport_image.destroy();
        viewport_image = plot_image;
      }
    }
    else if (current_stage == ReconstructionStage::FaceExtraction && worker_state == ThreadState::Running && worker_is_done.load())
    {
      worker_state = ThreadState::WaitingConfirmation;
      worker_is_done = false;
      if(std::filesystem::exists("faces.png"))
      {
        auto plot_image = Texture::create_from_file("faces.png");
        if(viewport_image.is_valid())
          viewport_image.destroy();
        viewport_image = plot_image;
      }
    }
    else if (current_stage == ReconstructionStage::BuildMesh && worker_state == ThreadState::Running && worker_is_done.load())
    {
      worker_state = ThreadState::WaitingConfirmation;
      worker_is_done = false;
      static_mesh = std::make_shared<StaticMesh>(
        build_result.mesh_vertices.data(), build_result.mesh_vertices.size(),
        build_result.mesh_indices.data(), build_result.mesh_indices.size()
      );
    }

    // =======================================================
    // Viewport panel
    // =======================================================

    auto new_info = viewport_panel(viewport_image);
    if(viewport_info.width != new_info.width || viewport_info.height != new_info.height)
    {      
      viewport_info = new_info;
      viewport_framebuffer.destroy();
      fb_color_texture.destroy();
      fb_depth_texture.destroy();
      create_framebuffer(viewport_framebuffer, fb_color_texture, fb_depth_texture, static_cast<i32>(viewport_info.width), static_cast<i32>(viewport_info.height));
    }
    
    glViewport(0, 0, viewport_info.width, viewport_info.height);
    camera.aspect = viewport_info.aspect;
    
    // =======================================================
    // Log panel
    // =======================================================
    
    log_panel(window_context, current_stage, worker_state);
    
    // =======================================================
    // Properties panel
    // =======================================================
    properties_panel();
    
    // =======================================================
    // rendering mesh with shaders
    // =======================================================
    if(static_mesh)
    {
      handle_camera_input(window_context, camera);   
      auto mat_camera = camera.canonical_to_camera();
      auto mat_persp = camera.get_perspective();

      pipeline.bind();
      pipeline.set_active_program(vertex_program);
      vertex_program.set_uniform_mat4f(1, &mat_camera[0][0]);
      vertex_program.set_uniform_mat4f(2, &mat_persp[0][0]);
      vertex_program.set_uniform_mat4f(0, &mesh_transform.M[0][0]);
      static_mesh->vao().bind();
      if(static_mesh->nr_indices() > 0)
        glDrawElements(GL_TRIANGLES, static_mesh->nr_indices(), GL_UNSIGNED_INT, 0);
      else
        glDrawArrays(GL_TRIANGLES, 0, static_mesh->nr_vertices());
    }
     
    render_gui(); 
    glfwSwapBuffers(window_context);
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