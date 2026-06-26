#include "gui.hpp"
#include "../graphics/texture.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <print>

GLFWwindow* init_window_context(i32 width, i32 height)
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

  std::println("OpenGL context initialized successfully. Version: {}.{}",  GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
  
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

ViewportInfo viewport_panel(Texture viewport_image, bool flip_viewport_image)
{
  ViewportInfo info{};
  ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoCollapse);

  auto window_size = ImGui::GetContentRegionAvail(); 
  info.width = static_cast<i32>(window_size.x);
  info.height = static_cast<i32>(window_size.y); 
  info.aspect = static_cast<f32>(info.width) / static_cast<f32>(info.height);
  if (viewport_image.is_valid())
  {
    auto uv0 = ImVec2{0,0,};
    auto uv1 = ImVec2{1,1};
    if(flip_viewport_image)
    {
      uv0 = ImVec2{0,1,};
      uv1 = ImVec2{1,0};
    }
    ImGui::Image(viewport_image.id(), window_size, uv0, uv1);
  }

  ImGui::End();

  return info;
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

void log_panel(GLFWwindow* window, ReconstructionStage& current_stage, std::atomic<ThreadState>& worker_state)
{
  ImGui::SetNextWindowSize(ImVec2(0, 200), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Log", nullptr, ImGuiWindowFlags_NoCollapse)) 
  {
    static std::vector<std::string> log_messages = {};
    if (ImGui::BeginChild("LogContent", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders, ImGuiWindowFlags_AlwaysVerticalScrollbar)) 
    {
      for (const auto& msg : log_messages) 
        ImGui::TextColored(ImVec4(1, 1, 1, 0.8f), "> %s", msg.c_str());
      if (worker_state == ThreadState::WaitingConfirmation) 
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Continuare? [y/n]");
      if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    static auto input_buf = std::array<char, 2>{};
    ImGui::PushItemWidth(-1);
    auto reclaim_focus = false;
    if (ImGui::InputText("##ConsoleInput", input_buf.data(), 2, ImGuiInputTextFlags_EnterReturnsTrue)) 
    {
      log_messages.push_back(input_buf.data());
      if (worker_state == ThreadState::WaitingConfirmation) 
      {
        if (input_buf.at(0) == 'y' || input_buf.at(0) == 'Y')
        {
          log_messages.push_back("Confermato, avanzo alla fase successiva.");
          current_stage = next_stage(current_stage);
          worker_state = ThreadState::Idle;
        } 
        else if (input_buf.at(0) == 'n' || input_buf.at(0) == 'N')
        {
          log_messages.push_back("Annullato dall'utente.");
          glfwSetWindowShouldClose(window, true);
        } 
        else 
        {
          log_messages.push_back("Risposta non valida, digita 'y' o 'n'.");
        }
      } 
      input_buf.fill(0);
      reclaim_focus = true;
    }
    ImGui::PopItemWidth();

    ImGui::SetItemDefaultFocus();
    if (reclaim_focus)
      ImGui::SetKeyboardFocusHere(-1);
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