#include "config_loader.hpp"

#include <cassert>
#include <nlohmann/json.hpp>
#include <fstream>
#include <format>

Config::Config(const std::filesystem::path& config_path)
{
  auto file = std::ifstream(config_path);
  if (!file.is_open()) 
    throw std::runtime_error(std::format("Error on opening configuration file: {}", config_path.string()));

  auto json = nlohmann::json{};
  file >> json;

  auto filename = json.value("dxf_filename", "");
  if(filename.empty())
    throw std::runtime_error("Missing dxf input file");

  dxf_path              = config_path.parent_path() / filename;
  unit_scale            = json.value("unit_scale", 1.0);

  ceil_height           = json.value("ceil_height", 2.7f);
  door_width            = json.value("door_width", 1.2f);
  door_height           = json.value("door_height", 2.1f);
  assert(door_height < ceil_height && "Door height must be less than ceiling height");

  window_sill_height    = json.value("window_sill_height", 0.9f);
  window_height         = json.value("window_height", 1.2f);
  window_width          = json.value("window_width", 1.0f);
  assert(window_sill_height < window_height && "Window sill height must be less than window height");
  assert(window_height < ceil_height && "Window height must be less than ceiling height");
  
  snap_eps              = json.value("snap_eps", 1e-2);
  cluster_num_samples   = json.value("cluster_num_samples", 10);
  cluster_eps           = json.value("cluster_eps", 1.0);
  
  floor_texture_scaling = json.value("floor_texture_scaling", 1.0f);
  wall_texture_scaling  = json.value("wall_texture_scaling", 1.0f);
}