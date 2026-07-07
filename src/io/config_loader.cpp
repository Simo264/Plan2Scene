#include "config_loader.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
#include <format>

Config load_config(std::string_view config_file)
{
  auto file = std::ifstream(config_file.data());
  if (!file.is_open()) 
    throw std::runtime_error(std::format("Error on opening configuration file: {}", config_file));

  auto json = nlohmann::json{};
  file >> json;

  auto filename = json.value("dxf_filename", "");
  if(filename.empty())
  throw std::runtime_error("Missing dxf input file");

  auto cfg = Config{};

  auto config_dir = std::filesystem::path(config_file).parent_path();
  cfg.dxf_path = config_dir / filename;

  cfg.unit_scale            = json.value("unit_scale", 1.0);
  cfg.snap_eps              = json.value("snap_eps", 1e-4);
  cfg.cluster_num_samples   = json.value("cluster_num_samples", 10);
  cfg.cluster_eps           = json.value("cluster_eps", 1.0);
  cfg.ceil_height           = json.value("ceil_height", 10.0f);
  cfg.floor_texture_scaling = json.value("floor_texture_scaling", 2.0f);
  cfg.wall_texture_scaling  = json.value("wall_texture_scaling", 2.0f);

  cfg.door_frac_top         = 0.8f * cfg.ceil_height; // from 80% to 100%
  cfg.window_frac_bottom    = 0.2f * cfg.ceil_height; // from 0% to 20%
  cfg.window_frac_top       = 0.8f * cfg.ceil_height; // from 80% to 100%
  return cfg;
}