#pragma once

#include "../types.hpp"

#include <filesystem>
#include <string_view>

struct Config 
{
  std::filesystem::path dxf_path;

  f64 unit_scale;

  f64 snap_eps;
  
  i32 cluster_num_samples;
  f64 cluster_eps;
  
  f32 ceil_height;
  f32 door_frac_top;
  f32 window_frac_bottom;
  f32 window_frac_top;
  f32 floor_texture_scaling;
  f32 wall_texture_scaling;
};

Config load_config(std::string_view config_file);