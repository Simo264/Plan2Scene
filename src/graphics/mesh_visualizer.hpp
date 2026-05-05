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
  void render();
  
private:
  auto init_context(i32 width, i32 height) -> struct GLFWwindow*;
  void create_pipeline_object();
  void handle_camera_input();
  void prepare_mesh();

  struct GLFWwindow* m_context;
  Camera m_camera;
 
  ShaderProgram m_vertex_program;
  ShaderProgram m_fragment_program;
  ProgramPipelineObject m_pipeline_object;

  std::unique_ptr<StaticMesh> m_mesh;
  Transformation m_mesh_transform;
};