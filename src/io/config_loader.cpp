#include "config_loader.hpp"

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
  door_width            = json.value("door_width", 10.0f);
  ceil_height           = json.value("ceil_height", 10.0f);

  snap_eps              = json.value("snap_eps", 1e-4);
  cluster_num_samples   = json.value("cluster_num_samples", 10);
  cluster_eps           = json.value("cluster_eps", 1.0);
  floor_texture_scaling = json.value("floor_texture_scaling", 2.0f);
  wall_texture_scaling  = json.value("wall_texture_scaling", 2.0f);
}