#pragma once

#include "../types.hpp"
#include "../reconstruction.hpp"

struct ViewportInfo
{
  i32 width, height;
  f32 aspect;
};


struct GLFWwindow* init_window_context(i32 width, i32 height);

void setup_docking();

ViewportInfo viewport_panel(class Texture viewport_image, bool flip_viewport_image);

void log_panel(GLFWwindow* window, ReconstructionStage& current_stage, std::atomic<ThreadState>& worker_state);
void properties_panel();

void render_gui();