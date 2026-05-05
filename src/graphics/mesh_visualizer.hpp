#pragma once

#include "../types.hpp"

#include "camera.hpp"
#include "pipeline.hpp"
#include "static_mesh.hpp"
#include "transformation.hpp"

#include <memory>

class MeshVisualizer
{
public:
  MeshVisualizer(i32 width, i32 height);
  void set_mesh(std::shared_ptr<StaticMesh> mesh) { m_mesh = mesh; }
  void set_mesh_transform(const Transformation& transform) { m_mesh_transform = transform; }
  void render();
  auto camera() -> Camera& { return m_camera; }
  
private:
  auto init_context(i32 width, i32 height) -> struct GLFWwindow*;
  void create_pipeline_object();
  void handle_camera_input();

  struct GLFWwindow* m_context;
  Camera m_camera;
 
  ShaderProgram m_vertex_program;
  ShaderProgram m_fragment_program;
  ProgramPipelineObject m_pipeline_object;

  std::shared_ptr<StaticMesh> m_mesh;
  Transformation m_mesh_transform;
};