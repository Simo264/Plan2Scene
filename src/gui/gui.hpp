#pragma once

#include "../types.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "../reconstruction.hpp"

struct ViewportInfo
{
  f32 width;
  f32 height;
  f32 aspect;
};


GLFWwindow* init_window_context(i32 width, i32 height);

void setup_docking();

ViewportInfo viewport_panel(class Texture viewport_image);

void log_panel(GLFWwindow* window, ReconstructionStage& current_stage, std::atomic<ThreadState>& worker_state);
void properties_panel();

void render_gui();