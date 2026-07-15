#pragma once

#include "../types.hpp"

#include <filesystem>

struct Config 
{
  Config() = default;
  
  // e.g: "cad/house_plan/draftperson_Floor_Plan.json"
  Config(const std::filesystem::path& config_path);

  std::filesystem::path dxf_path;

  f64 unit_scale;
  f32 door_width;
  f32 ceil_height;

  f64 snap_eps;
  
  i32 cluster_num_samples;
  f64 cluster_eps;
  
  f32 floor_texture_scaling;
  f32 wall_texture_scaling;
};