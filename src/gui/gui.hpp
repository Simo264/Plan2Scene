#pragma once

#include "../types.hpp"
#include "../reconstruction.hpp"
#include <string_view>

struct ViewportInfo
{
  i32 width, height;
  glm::vec2 screen_pos;
  f32 aspect;
};

struct GLFWwindow* init_window_context(i32 width, 
                                       i32 height);

void setup_docking();

ViewportInfo viewport_panel(class Texture viewport_image, 
                            bool flip_viewport_image);

void console_panel(GLFWwindow* window, 
                   ReconstructionStage& current_stage, 
                   std::atomic<ThreadState>& worker_state);

void mesh_details_overlay(const class StaticMesh& mesh, glm::vec2 viewport_pos);

void properties_panel(const std::string_view& file_name, 
                      f64 snap_eps, 
                      i32 cluster_num_samples, 
                      f64 cluster_eps,
                      glm::vec3& light_pos);

void render_gui();