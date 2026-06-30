#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <exception>
#include <filesystem>
#include <format>
#include <memory>
#include <stdexcept>
#include <atomic>

#include "types.hpp"
#include "geometry.hpp"
#include "reconstruction.hpp"
#include "log.hpp"
#include "graphics/texture.hpp"
#include "graphics/pipeline.hpp"
#include "graphics/camera.hpp"
#include "graphics/static_mesh.hpp"
#include "graphics/transformation.hpp"
#include "graphics/framebuffer.hpp"
#include "io/gltf_exporter.hpp"
#include "gui/gui.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/trigonometric.hpp>
#include <glm/geometric.hpp>

constexpr auto window_width = 1024;
constexpr auto window_height = 768;
constexpr auto snap_eps = 1e-4;
constexpr auto cluster_num_samples = 20;
constexpr auto cluster_eps = 0.1;

 static auto viewport_info = ViewportInfo{
   .width=window_width, 
   .height=window_height, 
   .screen_pos=glm::vec2{0, 0},
   .aspect=static_cast<f32>(window_width) / static_cast<f32>(window_height)
 };

static auto vertex_program = ShaderProgram{};
static auto fragment_program = ShaderProgram{};
static auto pipeline = ProgramPipelineObject{};

static auto fbo = FrameBuffer{};
static auto fbo_color_texture = Texture{};
static auto fbo_depth_texture = Texture{};

static auto camera = Camera(0.1f, 100.0f, 45.f, viewport_info.aspect);
static auto light_pos = glm::vec3(2, 4.0, 0);
static auto static_mesh = std::unique_ptr<StaticMesh>{};
static auto mesh_transform = Transformation{};

static auto viewport_image = Texture{};
static auto plot_image = Texture{};

static auto current_stage = ReconstructionStage::PrimitivesExtraction;
static auto worker_state = std::atomic<ThreadState>{ ThreadState::Idle };
static auto worker = std::optional<std::jthread>{};
static auto worker_is_done = std::atomic<bool>{ false };
static auto build_result = ReconstructionResult{};
static auto ctx = ReconstructionContext{};

auto g_logger = Logger{};

static void create_gl_pipeline_object(ShaderProgram& vertex_program, ShaderProgram& fragment_program, ProgramPipelineObject& pipeline)
{
  auto shaders_dir = std::filesystem::current_path() / "shaders";

  auto vertex_shader_obj = ShaderObject{};
  vertex_shader_obj.create(ShaderStage::Vertex);
  vertex_shader_obj.load_source_code(shaders_dir / "basic_shader.vert.glsl");
  vertex_shader_obj.compile();
  auto status = vertex_shader_obj.check_compile_status();
  if (!status)
    throw std::runtime_error(std::format("Shader compilation error: {}", vertex_shader_obj.get_compile_log()));

  auto fragment_shader_obj = ShaderObject{};
  fragment_shader_obj.create(ShaderStage::Fragment);
  fragment_shader_obj.load_source_code(shaders_dir / "basic_shader.frag.glsl");
  fragment_shader_obj.compile();
  status = fragment_shader_obj.check_compile_status();
  if (!status)
    throw std::runtime_error(std::format("Shader compilation error: {}", fragment_shader_obj.get_compile_log()));

  vertex_program.create();
  vertex_program.attach_shader(vertex_shader_obj);
  vertex_program.set_separable(true);
  vertex_program.link();
  status = vertex_program.check_link_status();
  if (!status)
    throw std::runtime_error(std::format("Link status: {}", vertex_program.get_link_log()));

  vertex_program.detach_shader(vertex_shader_obj);

  fragment_program.create();
  fragment_program.attach_shader(fragment_shader_obj);
  fragment_program.set_separable(true);
  fragment_program.link();
  status = fragment_program.check_link_status();
  if (!status)
    throw std::runtime_error(std::format("Link status: {}", fragment_program.get_link_log()));

  fragment_program.detach_shader(fragment_shader_obj);

  pipeline.create();
  pipeline.bind_program_stage(PipelineStage::VertexShader, vertex_program);
  pipeline.bind_program_stage(PipelineStage::FragmentShader, fragment_program);
  status = pipeline.validate_pipeline();
  if (!status)
    throw std::runtime_error(std::format("pipeline object status: {}", pipeline.get_validation_status()));
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
  // --- Color texture ---
  color.create(TextureType::Texture2D);
  color.set_storage_tex2D(1, TextureImageFormat::RGBA8, width, height);
  color.set_wrap_mode(TextureWrapMode::ClampToEdge, TextureWrapMode::ClampToEdge);
  color.set_magnification_filter(TextureFilteringMode::Linear);
  color.set_minification_filter(TextureFilteringMode::Linear);

  // --- Depth texture  ---
  depth.create(TextureType::Texture2D);
  depth.set_storage_tex2D(1, TextureImageFormat::Depth24Stencil8, width, height);
  depth.set_wrap_mode(TextureWrapMode::ClampToEdge, TextureWrapMode::ClampToEdge);
  depth.set_magnification_filter(TextureFilteringMode::Nearest);
  depth.set_minification_filter(TextureFilteringMode::Nearest);

  fb.create();
  fb.bind(FramebufferTarget::READ_DRAW);
  fb.attach_texture(FramebufferAttachment::COLOR_0, color, 0);
  fb.attach_texture(FramebufferAttachment::DEPTH_STENCIL, depth, 0);

  if (!fb.check_status()) 
    throw std::runtime_error("Invalid framebuffer object!");
  
  fb.unbind(FramebufferTarget::READ_DRAW);
}

int main(int argc, char* argv[])
{
  if(argc != 2)
    throw std::runtime_error("Usage: ./build/Plan2Scene <input.dxf>");
  
  auto file_path = std::filesystem::path(argv[1]);
  auto filename = file_path.string();
  if(!std::filesystem::exists(file_path))
    throw std::runtime_error(std::format("Input file not found: {}", file_path.string()));

  auto window_context = init_window_context(viewport_info.width, viewport_info.height);
 
  create_gl_pipeline_object(vertex_program, fragment_program, pipeline);

  camera.eye = { 0.f, 5.f, 10.f };
  camera.set_orientation(glm::radians(glm::vec3{ -20.f, 0.f, 0.f }));
 
  auto floor_texture = Texture::create_from_file("materials/interior_tiles/interior_tiles_diff_1k.jpg");
  auto wall_texture = Texture::create_from_file("materials/concrete_layers/concrete_layers_diff_1k.jpg");
  
  g_logger.push_message({ std::format("Processing CAD file: {}", file_path.string()), LogLevel::Text });
  while (!glfwWindowShouldClose(window_context))
  {
    glfwPollEvents();
    if (glfwGetKey(window_context, GLFW_KEY_ESCAPE) == GLFW_PRESS) 
      glfwSetWindowShouldClose(window_context, GLFW_TRUE);

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    setup_docking();

    // =======================================================
    // Start worker if idle
    // =======================================================

    if(worker_state == ThreadState::Idle)
    {
      switch (current_stage) 
      {
        // =======================================================
        // Primitives extraction
        // =======================================================
        case ReconstructionStage::PrimitivesExtraction:
        {
          g_logger.push_message({"[worker] Starting PrimitivesExtraction...", LogLevel::Info});
          worker_state = ThreadState::Running;
          worker.emplace([&] {
            try
            {
              Reconstruction::primitives_extraction(ctx, file_path);
              Reconstruction::checkpoint_raw_segments(ctx.walls, ctx.doors, ctx.windows);
              worker_is_done = true;
            }
            catch(const std::exception& e)
            {
              g_logger.push_message({std::format("[worker] Error during PrimitivesExtraction.\n{}", e.what()), LogLevel::Error});
              worker_state = ThreadState::Error;
              worker_is_done = true;
            }
          });
          break;
        }

        // =======================================================
        // Vertex snapping
        // =======================================================  
        case ReconstructionStage::VertexSnapping:
        {
          g_logger.push_message({"[worker] Starting VertexSnapping...", LogLevel::Info});
          worker_state = ThreadState::Running;
          worker.emplace([&] {
            try 
            {
              auto vertices_before = ctx.walls.size() * 2;

              Reconstruction::vertex_snapping(ctx, snap_eps);

              auto vertices_after = ctx.hash.vertices().size();
              auto merged = vertices_before - vertices_after;
              g_logger.push_message({
                std::format("{} vertices in -> {} vertices out ({} merged, eps={:.4f})",
                vertices_before, vertices_after, merged, snap_eps),
                LogLevel::Text});

              worker_is_done = true;
            } 
            catch (const std::exception& e) 
            {
              g_logger.push_message({std::format("[worker] Error during VertexSnapping.\n{}", e.what()), LogLevel::Error});
              worker_state = ThreadState::Error;
              worker_is_done = true;
            }
          });
          break;
        }

        // =======================================================
        // Clusters extraction
        // =======================================================  
        case ReconstructionStage::ClustersExtraction:
        {
          g_logger.push_message({"[worker] Starting ClustersExtraction...", LogLevel::Info});
          worker_state = ThreadState::Running;
          worker.emplace([&] {
            try 
            {
              Reconstruction::clusters_extraction(ctx, cluster_num_samples, cluster_eps);
              Reconstruction::checkpoint_clusters(ctx.sample_points, ctx.clusters);
              worker_is_done = true;
            } 
            catch (const std::exception& e) 
            {
              g_logger.push_message({std::format("[worker] Error during ClustersExtraction.\n{}", e.what()), LogLevel::Error});
              worker_state = ThreadState::Error;
              worker_is_done = true;
            }
          });
          break;
        }
        
        // =======================================================
        // Clusters extraction
        // =======================================================
        case ReconstructionStage::GapsReconstruction:
        {
          g_logger.push_message({"[worker] Starting GapsReconstruction...", LogLevel::Info});
          worker_state = ThreadState::Running;
          worker.emplace([&] {
            try 
            {
              auto edges_before = ctx.edges.size();
              auto doors_count = ctx.doors.size();
              auto windows_count = ctx.clusters.size();

              Reconstruction::gaps_reconstruction(ctx);

              auto edges_after = ctx.edges.size();
              auto edges_added = edges_after - edges_before;

              g_logger.push_message({
                std::format("{} doors, {} window clusters -> {} edges added (total edges: {})",
                doors_count, windows_count, edges_added, edges_after),
                LogLevel::Text });

              worker_is_done = true;
            } 
            catch (const std::exception& e) 
            {
              g_logger.push_message({std::format("[worker] Error during GapsReconstruction.\n{}", e.what()), LogLevel::Error});
              worker_state = ThreadState::Error;
              worker_is_done = true;
            }
          });
          break;
        }

        // =======================================================
        // Faces extraction
        // =======================================================
        case ReconstructionStage::FacesExtraction:
        {
          g_logger.push_message({"[worker] Starting FaceExtraction...", LogLevel::Info});
          worker_state = ThreadState::Running;
          worker.emplace([&] {
            try 
            {
              Reconstruction::faces_extraction(ctx, ctx.hash.vertices(), ctx.edges);
              g_logger.push_message({std::format("Arrangement completed:\n number of vertices={}\n number of edges={}\n number of faces={}",
                  ctx.arrangement.number_of_vertices(), ctx.arrangement.number_of_edges(), ctx.arrangement.number_of_faces()), LogLevel::Text});
              g_logger.push_message({ std::format("Number of extracted faces: {}", ctx.faces.size()), LogLevel::Text});

              Reconstruction::checkpoint_faces(ctx.faces);
              worker_is_done = true;
            } 
            catch (const std::exception& e) 
            {
              g_logger.push_message({std::format("[worker] Error during FaceExtraction.\n{}", e.what()), LogLevel::Error});
              worker_state = ThreadState::Error;
              worker_is_done = true;
            } 
          });
          break;
        } 

        // =======================================================
        // Mesh building
        // =======================================================
        case ReconstructionStage::BuildMesh:
        {
          g_logger.push_message({"[worker] Starting BuildMesh...", LogLevel::Info});
          worker_state = ThreadState::Running;
          worker.emplace([&] {
            try 
            {
              // remove all FLOOR faces and push only one quad for floor
              std::erase_if(ctx.faces, [](auto face) { return face.type == FaceType::FLOOR; });
              auto house_bbox = calculate_bbox_2D(ctx.walls);
              auto floor_face = Face{};
              floor_face.vertices = {
                glm::dvec2(house_bbox.min.x, house_bbox.min.y),
                glm::dvec2(house_bbox.max.x, house_bbox.min.y),
                glm::dvec2(house_bbox.max.x, house_bbox.max.y),
                glm::dvec2(house_bbox.min.x, house_bbox.max.y) 
              };
              floor_face.type = FaceType::FLOOR;
              ctx.faces.push_back(std::move(floor_face));

              build_result = Reconstruction::build_mesh(ctx.faces);
              worker_is_done = true;
            } 
            catch (const std::exception& e) 
            {
              g_logger.push_message({std::format("[worker] Error during BuildMesh.\n{}", e.what()), LogLevel::Error});
              worker_state = ThreadState::Error;
              worker_is_done = true;
            }
          });
          break;
        }

        default: break;
      }
    }

    // =======================================================
    // Worker completion (Running -> WaitingConfirmation)
    // =======================================================

    else if(worker_state == ThreadState::Running && worker_is_done.load())
    {
      worker_is_done = false;
      switch (current_stage) 
      {
        // ===========================================
        // On PrimitivesExtraction completed
        // ===========================================
        case ReconstructionStage::PrimitivesExtraction:
        {
          g_logger.push_message({"PrimitivesExtraction completed! Loading segments.png...", LogLevel::Success});
          if(std::filesystem::exists("segments.png"))
          {
            if(plot_image.is_valid()) plot_image.destroy();
            
            plot_image = Texture::create_from_file("segments.png");
            viewport_image = plot_image;
          }
          break;
        }

        // ===========================================
        // On VertexSnapping completed
        // ===========================================
        case ReconstructionStage::VertexSnapping:
        {
          g_logger.push_message({"VertexSnapping completed!", LogLevel::Success});
          break; // no plot, no confirmation prompt
        }
        
        // ===========================================
        // On ClustersExtraction completed
        // ===========================================
        case ReconstructionStage::ClustersExtraction:
        {
          g_logger.push_message({"ClustersExtraction completed! Loading clusters.png...", LogLevel::Success});
          if(std::filesystem::exists("clusters.png"))
          {
            if(plot_image.is_valid()) plot_image.destroy();
            
            plot_image = Texture::create_from_file("clusters.png");
            viewport_image = plot_image;
          }
          break;
        }  
        
        // ===========================================
        // On GapsReconstruction completed
        // ===========================================
        case ReconstructionStage::GapsReconstruction:
        {
          g_logger.push_message({"GapsReconstruction completed!", LogLevel::Success});
          break; // no plot, no confirmation prompt
        }
        
        // ===========================================
        // On FacesExtraction completed
        // ===========================================
        case ReconstructionStage::FacesExtraction:
        {
          g_logger.push_message({"FaceExtraction completed! Loading faces.png...", LogLevel::Success});
          if(std::filesystem::exists("faces.png"))
          {
            if(plot_image.is_valid()) plot_image.destroy();
            
            plot_image = Texture::create_from_file("faces.png");
            viewport_image = plot_image;
          }
          break;
        }

        // ===========================================
        // On BuildMesh completed
        // ===========================================
        case ReconstructionStage::BuildMesh:
        {
          g_logger.push_message({"BuildMesh completed! Creating static_mesh...", LogLevel::Success});
          // Center the vertices at the origin. No transform needed.
          center_mesh(build_result.mesh_vertices);
          static_mesh = std::make_unique<StaticMesh>(build_result.mesh_vertices.data(),
                                                    build_result.mesh_vertices.size(),
                                                    build_result.mesh_indices.data(),
                                                    build_result.mesh_indices.size());

          auto gltf_path = file_path.filename().replace_extension("gltf");
          export_to_gltf(build_result.mesh_vertices, build_result.mesh_indices, gltf_path);
          g_logger.push_message({std::format("The exported model: {}", gltf_path.string()), LogLevel::Text});
          break;
        }

        default: 
          break;
      }

      if (stage_needs_confirmation(current_stage)) 
      {
        worker_state = ThreadState::WaitingConfirmation; // wait for "y/n" input from the console panel
      } 
      else 
      {
         // advance immediately without asking for confirmation
        current_stage = next_stage(current_stage);
        worker_state = ThreadState::Idle;
      }
    }

    // =======================================================
    // Viewport panel
    // =======================================================
    auto flip_viewport_image = (current_stage == ReconstructionStage::RenderMesh && static_mesh);
    auto new_info = viewport_panel(viewport_image, flip_viewport_image);
    if(viewport_info.width != new_info.width || viewport_info.height != new_info.height)
    {
      viewport_info = new_info;
      camera.aspect = viewport_info.aspect;
      if(current_stage == ReconstructionStage::RenderMesh && fbo.is_valid())
      {
        fbo.destroy();
        fbo_color_texture.destroy();
        fbo_depth_texture.destroy();
        create_framebuffer(fbo, fbo_color_texture, fbo_depth_texture, viewport_info.width, viewport_info.height);
      }
    }
    
    // =======================================================
    // Model rendering (RenderMesh only)
    // =======================================================
    if(current_stage == ReconstructionStage::RenderMesh && static_mesh)
    {
      if(!fbo.is_valid())
        create_framebuffer(fbo, fbo_color_texture, fbo_depth_texture, viewport_info.width, viewport_info.height);

      fbo.bind(FramebufferTarget::READ_DRAW);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glViewport(0, 0, viewport_info.width, viewport_info.height);

      handle_camera_input(window_context, camera);   
      auto mat_camera = camera.canonical_to_camera();
      auto mat_persp = camera.get_perspective();

      pipeline.bind();
      pipeline.set_active_program(vertex_program);
      vertex_program.set_uniform_mat4f(0, &mesh_transform.M[0][0]);
      vertex_program.set_uniform_mat4f(1, &mat_camera[0][0]);
      vertex_program.set_uniform_mat4f(2, &mat_persp[0][0]);
      pipeline.set_active_program(fragment_program);
      fragment_program.set_uniform_vector3f(0, &camera.eye[0]);
      fragment_program.set_uniform_vector3f(1, &light_pos[0]);

      static_mesh->vao.bind();
      //glDrawElements(GL_TRIANGLES, static_mesh->nr_indices(), GL_UNSIGNED_INT, 0);
      for (const auto& prim : build_result.primitives)
      {
        if(prim.material == MaterialType::Floor)
          floor_texture.bind_texture_unit(2);
        else
          wall_texture.bind_texture_unit(2);
        
        glDrawElements(GL_TRIANGLES, prim.index_count, GL_UNSIGNED_INT, (void*)(uintptr_t)(prim.index_offset * sizeof(u32)));
      } 

      fbo.unbind(FramebufferTarget::READ_DRAW);

      viewport_image = fbo_color_texture;

      mesh_details_overlay(*static_mesh, new_info.screen_pos);
    }

    // =======================================================
    // Log panel
    // =======================================================
    console_panel(window_context, current_stage, worker_state);
    
    // =======================================================
    // Properties panel
    // =======================================================
    properties_panel(filename, snap_eps, cluster_num_samples, cluster_eps, light_pos, mesh_transform);
    
    render_gui(); 
    glfwSwapBuffers(window_context);
  }
  glfwTerminate();
  
  if(plot_image.is_valid()) plot_image.destroy();
  if(fbo.is_valid())
  {
    fbo.destroy();
    fbo_color_texture.destroy();
    fbo_depth_texture.destroy();
  }
  
  return 0;
}