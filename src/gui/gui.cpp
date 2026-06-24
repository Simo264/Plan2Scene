#include "gui.hpp"

#include <print>

f32 aspect_ratio = 1.0f;

GLFWwindow* init_window_context(i32 width, i32 height)
{
  aspect_ratio = static_cast<f32>(width) / static_cast<f32>(height);

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

  std::println("OpenGL context initialized successfully. Version: {}.{}",  GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
  
  glfwSetFramebufferSizeCallback(context, []([[maybe_unused]] GLFWwindow* window, i32 width, i32 height) 
  {
    glViewport(0, 0, width, height);
    aspect_ratio = static_cast<f32>(width) / static_cast<f32>(height);
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

  glEnable(GL_DEPTH_TEST);  // enable depth testing
  glDepthFunc(GL_LESS);    	// specify the value used for depth buffer comparisons
  glDepthMask(GL_TRUE);    	// enable/disable writing into the depth buffer
  glClearDepthf(1.0f);      // specify the clear value for the depth buffer
  glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // specify the clear value for the color buffer
  
  return context;
}

void setup_docking()
{
  ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
  auto viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
  window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus; 
  
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));    
  ImGui::Begin("MainViewport", nullptr, window_flags);
  auto dockspace_id = ImGui::GetID("dockspace");
  ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
  ImGui::End();
  ImGui::PopStyleVar(3);
}

void viewport_panel()
{
  if (ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoCollapse)) 
  {
    ImVec2 avail_size = ImGui::GetContentRegionAvail();
    ImGui::Text("Rendering Area (3D Scene)");
    ImGui::Separator();
    ImGui::BeginChild("SceneView", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollWithMouse);
    {
      ImGui::Text("Mouse: (%.1f, %.1f)", ImGui::GetMousePos().x - ImGui::GetWindowPos().x, ImGui::GetMousePos().y - ImGui::GetWindowPos().y);
    }
    ImGui::EndChild();
  }
  ImGui::End();
}

void properties_panel()
{
  ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_FirstUseEver); // Larghezza fissa, altezza automatica
  if (ImGui::Begin("Properties", nullptr)) 
  {
    ImGui::Text("Properties Panel");
    ImGui::Separator();
    
    static float rotation_x = 0.0f;
    static float rotation_y = 0.0f;
    static float scale = 1.0f;
    static bool wireframe = false;

    ImGui::SliderFloat("Rotation X", &rotation_x, 0.0f, 360.0f);
    ImGui::SliderFloat("Rotation Y", &rotation_y, 0.0f, 360.0f);
    ImGui::SliderFloat("Scale", &scale, 0.1f, 5.0f);
    ImGui::Checkbox("Wireframe", &wireframe);
    
    ImGui::Separator();
    ImGui::Text("Selected Object: Cube"); // Statico per ora
  }
  ImGui::End();
}

void log_panel()
{
  ImGui::SetNextWindowSize(ImVec2(0, 200), ImGuiCond_FirstUseEver); // Altezza fissa, larghezza automatica
  if (ImGui::Begin("Log", nullptr, ImGuiWindowFlags_NoCollapse)) 
  {
    ImGui::Text("Console Log");
    ImGui::Separator();        
    static std::vector<std::string> log_messages = {
        "System initialized successfully.",
        "Shader compilation complete.",
        "Mesh loaded: cube.obj",
        "Waiting for user input..."
    };

    if (ImGui::BeginChild("LogContent", ImVec2(0, 0), ImGuiChildFlags_Borders, ImGuiWindowFlags_AlwaysVerticalScrollbar)) 
    {
      for (const auto& msg : log_messages) 
      {
        ImGui::TextColored(ImVec4(1, 1, 1, 0.8f), "> %s", msg.c_str());
      }
      // Scroll automatico se si aggiungono nuovi log
      if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) 
      {
        ImGui::SetScrollHereY(1.0f);
      }
    }
    ImGui::EndChild();
  }
  ImGui::End();
}


void render_gui()
{
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
}