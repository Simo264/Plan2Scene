#include "mesh_visualizer.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

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
