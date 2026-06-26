#include "gui.hpp"

#include "../log.hpp"
#include "../graphics/texture.hpp"
#include "../graphics/static_mesh.hpp"

#include <format>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

extern Logger g_logger;

static void setup_catppuccin_mocha_theme() 
{
  ImGuiStyle& style = ImGui::GetStyle();
  ImVec4* colors = style.Colors;

  // Catppuccin Mocha Palette
  // --------------------------------------------------------
  const ImVec4 base       = ImVec4(0.117f, 0.117f, 0.172f, 1.0f); // #1e1e2e
  const ImVec4 mantle     = ImVec4(0.109f, 0.109f, 0.156f, 1.0f); // #181825
  const ImVec4 surface0   = ImVec4(0.200f, 0.207f, 0.286f, 1.0f); // #313244
  const ImVec4 surface1   = ImVec4(0.247f, 0.254f, 0.337f, 1.0f); // #3f4056
  const ImVec4 surface2   = ImVec4(0.290f, 0.301f, 0.388f, 1.0f); // #4a4d63
  const ImVec4 overlay0   = ImVec4(0.396f, 0.403f, 0.486f, 1.0f); // #65677c
  const ImVec4 overlay2   = ImVec4(0.576f, 0.584f, 0.654f, 1.0f); // #9399b2
  const ImVec4 text       = ImVec4(0.803f, 0.815f, 0.878f, 1.0f); // #cdd6f4
  const ImVec4 subtext0   = ImVec4(0.639f, 0.658f, 0.764f, 1.0f); // #a3a8c3
  const ImVec4 mauve      = ImVec4(0.796f, 0.698f, 0.972f, 1.0f); // #cba6f7
  const ImVec4 peach      = ImVec4(0.980f, 0.709f, 0.572f, 1.0f); // #fab387
  const ImVec4 yellow     = ImVec4(0.980f, 0.913f, 0.596f, 1.0f); // #f9e2af
  const ImVec4 green      = ImVec4(0.650f, 0.890f, 0.631f, 1.0f); // #a6e3a1
  const ImVec4 teal       = ImVec4(0.580f, 0.886f, 0.819f, 1.0f); // #94e2d5
  const ImVec4 sapphire   = ImVec4(0.458f, 0.784f, 0.878f, 1.0f); // #74c7ec
  const ImVec4 blue       = ImVec4(0.533f, 0.698f, 0.976f, 1.0f); // #89b4fa
  const ImVec4 lavender   = ImVec4(0.709f, 0.764f, 0.980f, 1.0f); // #b4befe

  // Main window and backgrounds
  colors[ImGuiCol_WindowBg]             = base;
  colors[ImGuiCol_ChildBg]              = base;
  colors[ImGuiCol_PopupBg]              = surface0;
  colors[ImGuiCol_Border]               = surface1;
  colors[ImGuiCol_BorderShadow]         = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  colors[ImGuiCol_FrameBg]              = surface0;
  colors[ImGuiCol_FrameBgHovered]       = surface1;
  colors[ImGuiCol_FrameBgActive]        = surface2;
  colors[ImGuiCol_TitleBg]              = mantle;
  colors[ImGuiCol_TitleBgActive]        = surface0;
  colors[ImGuiCol_TitleBgCollapsed]     = mantle;
  colors[ImGuiCol_MenuBarBg]            = mantle;
  colors[ImGuiCol_ScrollbarBg]          = surface0;
  colors[ImGuiCol_ScrollbarGrab]        = surface2;
  colors[ImGuiCol_ScrollbarGrabHovered] = overlay0;
  colors[ImGuiCol_ScrollbarGrabActive]  = overlay2;
  colors[ImGuiCol_CheckMark]            = green;
  colors[ImGuiCol_SliderGrab]           = sapphire;
  colors[ImGuiCol_SliderGrabActive]     = blue;
  colors[ImGuiCol_Button]               = surface0;
  colors[ImGuiCol_ButtonHovered]        = surface1;
  colors[ImGuiCol_ButtonActive]         = surface2;
  colors[ImGuiCol_Header]               = surface0;
  colors[ImGuiCol_HeaderHovered]        = surface1;
  colors[ImGuiCol_HeaderActive]         = surface2;
  colors[ImGuiCol_Separator]            = surface1;
  colors[ImGuiCol_SeparatorHovered]     = mauve;
  colors[ImGuiCol_SeparatorActive]      = mauve;
  colors[ImGuiCol_ResizeGrip]           = surface2;
  colors[ImGuiCol_ResizeGripHovered]    = mauve;
  colors[ImGuiCol_ResizeGripActive]     = mauve;
  colors[ImGuiCol_Tab]                  = surface0;
  colors[ImGuiCol_TabHovered]           = surface2;
  colors[ImGuiCol_TabActive]            = surface1;
  colors[ImGuiCol_TabUnfocused]         = surface0;
  colors[ImGuiCol_TabUnfocusedActive]   = surface1;
  colors[ImGuiCol_DockingPreview]       = sapphire;
  colors[ImGuiCol_DockingEmptyBg]       = base;
  colors[ImGuiCol_PlotLines]            = blue;
  colors[ImGuiCol_PlotLinesHovered]     = peach;
  colors[ImGuiCol_PlotHistogram]        = teal;
  colors[ImGuiCol_PlotHistogramHovered] = green;
  colors[ImGuiCol_TableHeaderBg]        = surface0;
  colors[ImGuiCol_TableBorderStrong]    = surface1;
  colors[ImGuiCol_TableBorderLight]     = surface0;
  colors[ImGuiCol_TableRowBg]           = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  colors[ImGuiCol_TableRowBgAlt]        = ImVec4(1.0f, 1.0f, 1.0f, 0.06f);
  colors[ImGuiCol_TextSelectedBg]       = surface2;
  colors[ImGuiCol_DragDropTarget]       = yellow;
  colors[ImGuiCol_NavHighlight]         = lavender;
  colors[ImGuiCol_NavWindowingHighlight]= ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
  colors[ImGuiCol_NavWindowingDimBg]    = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);
  colors[ImGuiCol_ModalWindowDimBg]     = ImVec4(0.0f, 0.0f, 0.0f, 0.35f);
  colors[ImGuiCol_Text]                 = text;
  colors[ImGuiCol_TextDisabled]         = subtext0;

  // Rounded corners
  style.WindowRounding    = 6.0f;
  style.ChildRounding     = 6.0f;
  style.FrameRounding     = 4.0f;
  style.PopupRounding     = 4.0f;
  style.ScrollbarRounding = 9.0f;
  style.GrabRounding      = 4.0f;
  style.TabRounding       = 4.0f;

  // Padding and spacing
  style.WindowPadding     = ImVec2(8.0f, 8.0f);
  style.FramePadding      = ImVec2(5.0f, 3.0f);
  style.ItemSpacing       = ImVec2(8.0f, 4.0f);
  style.ItemInnerSpacing  = ImVec2(4.0f, 4.0f);
  style.IndentSpacing     = 21.0f;
  style.ScrollbarSize     = 14.0f;
  style.GrabMinSize       = 10.0f;

  // Borders
  style.WindowBorderSize  = 1.0f;
  style.ChildBorderSize   = 1.0f;
  style.PopupBorderSize   = 1.0f;
  style.FrameBorderSize   = 0.0f;
  style.TabBorderSize     = 0.0f;
}

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

  g_logger.push_message({"GLFW window created successfully.", LogLevel::Text});
  
  glfwMakeContextCurrent(context);
  auto version = gladLoadGL(glfwGetProcAddress);
  if(!version)
    throw std::runtime_error("Failed to initialize OpenGL context");

  g_logger.push_message({
    std::format("OpenGL context initialized successfully. Version: {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version)),
    LogLevel::Text
  });
  
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  auto &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;           // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking

  auto* font = io.Fonts->AddFontFromFileTTF("fonts/Lato-Regular.ttf", 14.0f);
  if (!font) 
    g_logger.push_message({"Failed to load font 'fonts/Lato-Regular.ttf', using default.", LogLevel::Warning});
  
  ImGui::StyleColorsDark();
  setup_catppuccin_mocha_theme();

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

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoCollapse);
  ImGui::PopStyleVar();
    
  auto cursor_pos = ImGui::GetCursorScreenPos();
  info.screen_pos = glm::vec2(cursor_pos.x, cursor_pos.y);
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

void properties_panel(const std::string_view& file_name, f64 snap_eps, i32 cluster_num_samples, f64 cluster_eps) 
{
  ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Properties", nullptr)) 
  {
    ImGui::TextDisabled("Input");
    ImGui::Separator();
    ImGui::Columns(2, "props_input", false);
    ImGui::SetColumnWidth(0, 160.0f);
    ImGui::Text("File path:"); ImGui::NextColumn();
    ImGui::TextWrapped("%s", file_name.data()); ImGui::NextColumn();
    ImGui::Columns(1);

    ImGui::Spacing();
    ImGui::TextDisabled("Vertex snapping");
    ImGui::Separator();
    ImGui::Columns(2, "props_snap", false);
    ImGui::SetColumnWidth(0, 160.0f);
    ImGui::Text("Snap tolerance:"); ImGui::NextColumn();
    ImGui::Text("%.6f", snap_eps); ImGui::NextColumn();
    ImGui::Columns(1);

    ImGui::Spacing();
    ImGui::TextDisabled("Opening reconstruction");
    ImGui::Separator();
    ImGui::Columns(2, "props_cluster", false);
    ImGui::SetColumnWidth(0, 160.0f);
    ImGui::Text("Cluster samples:"); ImGui::NextColumn();
    ImGui::Text("%d", cluster_num_samples); ImGui::NextColumn();
    ImGui::Text("Cluster eps:"); ImGui::NextColumn();
    ImGui::Text("%.4f", cluster_eps); ImGui::NextColumn();
    ImGui::Columns(1);
  }
  ImGui::End();
}

void console_panel(GLFWwindow* window, ReconstructionStage& current_stage, std::atomic<ThreadState>& worker_state)
{
  auto log_level_to_color = [](LogLevel level) -> ImVec4 {
    auto hex = static_cast<u32>(level);
    auto r = ((hex >> 16) & 0xFF) / 255.0f;
    auto g = ((hex >> 8)  & 0xFF) / 255.0f;
    auto b = ( hex        & 0xFF) / 255.0f;
    return ImVec4(r, g, b, 1.0f);
  };
  
  ImGui::SetNextWindowSize(ImVec2(0, 200), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Console", nullptr, ImGuiWindowFlags_NoCollapse)) 
  {
    if (ImGui::BeginChild("LogContent", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders, ImGuiWindowFlags_AlwaysVerticalScrollbar)) 
    {
      auto messages = g_logger.get_messages();
      for (const auto& msg : messages) 
        ImGui::TextColored(log_level_to_color(msg.level), "> %s", msg.message.c_str());
      if (worker_state == ThreadState::WaitingConfirmation) 
        ImGui::TextColored(log_level_to_color(LogLevel::Text), "Proceed? [y/n]");
      if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    static auto input_buf = std::array<char, 2>{};
    ImGui::PushItemWidth(-1);
    auto reclaim_focus = false;
    if (ImGui::InputText("##ConsoleInput", input_buf.data(), 2, ImGuiInputTextFlags_EnterReturnsTrue)) 
    {
      if (worker_state == ThreadState::WaitingConfirmation) 
      {
        if (input_buf.at(0) == 'y' || input_buf.at(0) == 'Y')
        {
          current_stage = next_stage(current_stage);
          worker_state = ThreadState::Idle;
        } 
        else if (input_buf.at(0) == 'n' || input_buf.at(0) == 'N')
        {
          glfwSetWindowShouldClose(window, true);
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

void mesh_details_overlay(const StaticMesh& mesh, glm::vec2 viewport_pos) 
{
  constexpr auto margin = 12.0f;

  ImGui::SetNextWindowPos(ImVec2(viewport_pos.x + margin, viewport_pos.y + margin));
  ImGui::SetNextWindowSize(ImVec2(220.0f, 0.0f), ImGuiCond_Always);
  ImGui::SetNextWindowBgAlpha(0.0f);

  constexpr ImGuiWindowFlags flags =
    ImGuiWindowFlags_NoDecoration |
    ImGuiWindowFlags_NoBackground |
    ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoSavedSettings |
    ImGuiWindowFlags_AlwaysAutoResize |
    ImGuiWindowFlags_NoFocusOnAppearing |
    ImGuiWindowFlags_NoNav;

  if (ImGui::Begin("MeshPropsOverlay", nullptr, flags)) 
  {
    ImGui::Text("Geometry Data");
    ImGui::Separator();
    ImGui::Columns(2, "mesh_stats", false);
    ImGui::SetColumnWidth(0, 120.0f);
    ImGui::Text("Vertices:"); ImGui::NextColumn();
    ImGui::Text("%u", mesh.nr_vertices()); ImGui::NextColumn();
    ImGui::Text("Indices:"); ImGui::NextColumn();
    ImGui::Text("%u", mesh.nr_indices()); ImGui::NextColumn();
    ImGui::Columns(1);

    ImGui::Spacing();
    ImGui::Text("Video memory (VRAM)");
    ImGui::Separator();

    auto vbo_size = mesh.nr_vertices() * sizeof(Vertex_PN);
    auto ibo_size = mesh.nr_indices() * sizeof(u32);
    auto total_kb = (vbo_size + ibo_size) / 1024.0f;

    ImGui::Text("VBO Size:"); ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%.2f KB", vbo_size / 1024.0f);
    ImGui::Text("IBO Size:"); ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%.2f KB", ibo_size / 1024.0f);
    ImGui::Separator();
    ImGui::Text("Total:"); ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%.2f KB", total_kb);
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