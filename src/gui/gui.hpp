#pragma once

#include "../types.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

extern f32 aspect_ratio;

GLFWwindow* init_window_context(i32 width, i32 height);

void setup_docking();

void viewport_panel();
void properties_panel();
void log_panel();

void render_gui();