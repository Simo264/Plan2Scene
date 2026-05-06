#include "mesh_visualizer.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <print>
#include <filesystem>

static auto s_aspect_ratio = 1.f;

MeshVisualizer::MeshVisualizer(i32 width, i32 height)
{
  s_aspect_ratio = static_cast<f32>(width) / static_cast<f32>(height);
  
  m_context = init_context(width, height);

  m_camera = Camera(0.1f, 100.0f, 45.f, s_aspect_ratio);

  create_pipeline_object();
  
  glEnable(GL_DEPTH_TEST);  // enable depth testing
  glDepthFunc(GL_LESS);    	// specify the value used for depth buffer comparisons
  glDepthMask(GL_TRUE);    	// enable/disable writing into the depth buffer
  glClearDepthf(1.0f);      // specify the clear value for the depth buffer
  glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // specify the clear value for the color buffer
}

void MeshVisualizer::render()
{
  if(!m_mesh)
    return;
  
  while (!glfwWindowShouldClose(m_context)) 
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear buffers to preset values  

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    show_camera_props();
    show_mesh_props();

    handle_camera_input();   
    m_camera.aspect = s_aspect_ratio;
    auto mat_camera = m_camera.canonical_to_camera();
    auto mat_persp = m_camera.get_perspective();

    m_pipeline_object.bind();
    // set uniforms for vertex shader
    m_pipeline_object.set_active_program(m_vertex_program);
    m_vertex_program.set_uniform_mat4f(m_vertex_program.get_uniform_location("mat_cam"), &mat_camera[0][0]);
    m_vertex_program.set_uniform_mat4f(m_vertex_program.get_uniform_location("mat_per"), &mat_persp[0][0]);
    m_vertex_program.set_uniform_mat4f(m_vertex_program.get_uniform_location("mat_transform"), &m_mesh_transform.M[0][0]);
    // set uniforms for fragment shader
    m_pipeline_object.set_active_program(m_fragment_program);
    m_fragment_program.set_uniform_vector3f(m_fragment_program.get_uniform_location("u_camera_eye"), &m_camera.eye[0]); 
    
    m_mesh->vao().bind();
    if(m_mesh->nr_indices() > 0)
      glDrawElements(GL_TRIANGLES, m_mesh->nr_indices(), GL_UNSIGNED_INT, 0);
    else
      glDrawArrays(GL_TRIANGLES, 0, m_mesh->nr_vertices());

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    auto& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) 
    {
      auto backup_current_context = glfwGetCurrentContext();
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      glfwMakeContextCurrent(backup_current_context);
    }

    glfwSwapBuffers(m_context);
    glfwPollEvents();
  }
  
  glfwTerminate();
}



// ======== private methods ========
// =================================

auto MeshVisualizer::init_context(i32 width, i32 height) -> GLFWwindow*
{
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_DEPTH_BITS, 24);
  auto context = glfwCreateWindow(width, height, "Mesh visualizer", nullptr, nullptr);
  if(!context)
    throw std::runtime_error("Failed to create GLFW window");

  std::println("GLFW window created successfully.");
  
  glfwMakeContextCurrent(context);
  auto version = gladLoadGL(glfwGetProcAddress);
  if(!version)
    throw std::runtime_error("Failed to initialize OpenGL context");

  std::println("OpenGL context initialized successfully. Version: {}.{}", 
    GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
  
  glfwSetFramebufferSizeCallback(context, []([[maybe_unused]] GLFWwindow* window, i32 width, i32 height) {
    glViewport(0, 0, width, height);
    s_aspect_ratio = static_cast<f32>(width) / static_cast<f32>(height);
  });
  glViewport(0, 0, width, height);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  auto &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;           // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(context, true);
  ImGui_ImplOpenGL3_Init("#version 460");

  return context;
}

void MeshVisualizer::create_pipeline_object()
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

  m_vertex_program.create();
  m_vertex_program.attach_shader(vertex_shader_obj);
  m_vertex_program.set_separable(true);
  m_vertex_program.link();
  status = m_vertex_program.check_link_status();
  if (!status)
    std::println("Link status: {}", m_vertex_program.get_link_log());

  m_vertex_program.detach_shader(vertex_shader_obj);

  m_fragment_program.create();
  m_fragment_program.attach_shader(fragment_shader_obj);
  m_fragment_program.set_separable(true);
  m_fragment_program.link();
  status = m_fragment_program.check_link_status();
  if (!status)
    std::println("Link status: {}", m_fragment_program.get_link_log());

  m_fragment_program.detach_shader(fragment_shader_obj);

  m_pipeline_object.create();
  m_pipeline_object.bind_program_stage(PipelineStage::VertexShader, m_vertex_program);
  m_pipeline_object.bind_program_stage(PipelineStage::FragmentShader, m_fragment_program);
  status = m_pipeline_object.validate_pipeline();
  if (!status)
    std::println("pipeline object status: {}", m_pipeline_object.get_validation_status());
}

void MeshVisualizer::handle_camera_input()
{
  if (glfwGetKey(m_context, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(m_context, GLFW_TRUE);
  if (glfwGetKey(m_context, GLFW_KEY_UP) == GLFW_PRESS) m_camera.rotate_pitch(+glm::radians(1.0f));
  if (glfwGetKey(m_context, GLFW_KEY_DOWN) == GLFW_PRESS) m_camera.rotate_pitch(-glm::radians(1.0f));
  if (glfwGetKey(m_context, GLFW_KEY_LEFT) == GLFW_PRESS) m_camera.rotate_yaw(+glm::radians(1.0f));
  if (glfwGetKey(m_context, GLFW_KEY_RIGHT) == GLFW_PRESS) m_camera.rotate_yaw(-glm::radians(1.0f));
  if (glfwGetKey(m_context, GLFW_KEY_W) == GLFW_PRESS) m_camera.eye += m_camera.gaze() * 0.1f;
  if (glfwGetKey(m_context, GLFW_KEY_S) == GLFW_PRESS) m_camera.eye -= m_camera.gaze() * 0.1f;
  if (glfwGetKey(m_context, GLFW_KEY_A) == GLFW_PRESS) m_camera.eye -= m_camera.right() * 0.1f;
  if (glfwGetKey(m_context, GLFW_KEY_D) == GLFW_PRESS) m_camera.eye += m_camera.right() * 0.1f;
  if (glfwGetKey(m_context, GLFW_KEY_SPACE) == GLFW_PRESS) m_camera.eye += m_camera.up() * 0.1f;
  if (glfwGetKey(m_context, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) m_camera.eye -= m_camera.up() * 0.1f;
}

void MeshVisualizer::show_camera_props() 
{
  ImGui::SetNextWindowSize(ImVec2(350, 0), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Camera Control", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) 
  {
    ImGui::End();
    return;
  }

  auto fov = glm::degrees(m_camera.fovy);
  if (ImGui::SliderFloat("FOV", &fov, 30.0f, 120.0f, "%.1f°")) 
    m_camera.fovy = glm::radians(fov);

  ImGui::DragFloat("Near plane", &m_camera.near, 0.01f, 0.001f, 10.0f, "%.3f");
  ImGui::DragFloat("Far plane", &m_camera.far, 1.0f, 10.0f, 10000.0f, "%.1f");
  ImGui::InputFloat("Aspect ratio", &m_camera.aspect, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly);

  ImGui::Spacing();

  ImGui::DragFloat3("Position", &m_camera.eye.x, 0.1f);
  auto euler_deg = glm::degrees(m_camera.get_euler_angles());
  if (ImGui::DragFloat3("Orientation (deg)", &euler_deg.x, 0.5f, -180.0f, 180.0f, "%.1f°"))
    m_camera.set_orientation(glm::radians(euler_deg));

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  if (ImGui::Button("Reset Camera", ImVec2(ImGui::GetContentRegionAvail().x, 0))) 
  {
    m_camera.eye = glm::vec3(0.0f, 1.0f, 5.0f);
    m_camera.orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    m_camera.fovy = glm::radians(45.0f);
  }
  ImGui::End();
}

void MeshVisualizer::show_mesh_props() 
{
  ImGui::SetNextWindowSize(ImVec2(350, 0), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Mesh Properties", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) 
  {
    ImGui::End();
    return;
  }

  if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) 
  {
    auto changed = false;
    changed |= ImGui::DragFloat3("Position", &m_mesh_transform.position.x, 0.1f);
    
    auto rot_deg = glm::degrees(m_mesh_transform.rotation);
    if (ImGui::DragFloat3("Rotation", &rot_deg.x, 0.5f, -180.0f, 180.0f, "%.1f°"))
    {
      m_mesh_transform.rotation = glm::radians(rot_deg);
      changed = true;
    }
    
    changed |= ImGui::DragFloat3("Scale", &m_mesh_transform.scale.x, 0.05f, 0.001f, 100.0f);
    if (changed) 
      m_mesh_transform.update_tranformation();
  }

  ImGui::Spacing();

  if (ImGui::CollapsingHeader("Geometry Data", ImGuiTreeNodeFlags_DefaultOpen)) 
  {
    ImGui::Columns(2, "mesh_stats", false);
    ImGui::SetColumnWidth(0, 120.0f);
    ImGui::Text("Vertices:");   ImGui::NextColumn(); ImGui::Text("%u", m_mesh->nr_vertices()); ImGui::NextColumn();
    ImGui::Text("Indices:");    ImGui::NextColumn(); ImGui::Text("%u", m_mesh->nr_indices());  ImGui::NextColumn();
    ImGui::Columns(1);
  }

  ImGui::Spacing();

  if (ImGui::CollapsingHeader("Video memory (VRAM)")) 
  {
    auto vbo_size = m_mesh->nr_vertices() * sizeof(Vertex); // 24 bytes per vertice
    auto ibo_size = m_mesh->nr_indices() * sizeof(u32);     // 4 bytes per indice
    auto total_kb = (vbo_size + ibo_size) / 1024.0f;

    ImGui::Text("VBO Size:"); 
    ImGui::SameLine(); ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%.2f KB", vbo_size / 1024.0f);
    
    ImGui::Text("IBO Size:"); 
    ImGui::SameLine(); ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%.2f KB", ibo_size / 1024.0f);
    
    ImGui::Separator();
    ImGui::Text("Total:"); 
    ImGui::SameLine(); ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%.2f KB", total_kb);
  }

  ImGui::End();
}

