#include "config_loader.hpp"
#include "../globals.hpp"

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
  door_width            = json.value("door_width", 0.9f);
  door_height           = json.value("door_height", 2.1f);

  window_sill_height    = json.value("window_sill_height", 0.9f);
  window_height         = json.value("window_height", 1.4f);
  window_width          = json.value("window_width", 1.6f);
  
  snap_eps              = json.value("snap_eps", 1e-2);
  cluster_num_samples   = json.value("cluster_num_samples", 10);
  cluster_eps           = json.value("cluster_eps", 1.0);
  
  floor_texture_scaling = json.value("floor_texture_scaling", 1.0f);
  wall_texture_scaling  = json.value("wall_texture_scaling", 1.0f);

  validate_config();

  door_aspect = door_width / door_height;
  window_aspect = window_width / window_height;
  auto message = std::format("Configuration loaded successfully:\n"
    "  unit_scale               = {:.3f}\n"
    "  ceil_height              = {:.3f} m\n"
    "  door_width               = {:.3f} m\n"
    "  door_height              = {:.3f} m\n"
    "  door_aspect_ratio        = {:.3f}\n"
    "  window_sill_height       = {:.3f} m\n"
    "  window_height            = {:.3f} m\n"
    "  window_width             = {:.3f} m\n"
    "  window_aspect_ratio      = {:.3f}\n"
    "  snap_eps                 = {:.3e}\n"
    "  cluster_eps              = {:.3f}\n"
    "  cluster_num_samples      = {}\n"
    "  floor_texture_scaling    = {:.2f}\n"
    "  wall_texture_scaling     = {:.2f}",
    unit_scale,
    ceil_height,
    door_width,
    door_height,
    door_aspect,
    window_sill_height,
    window_height,
    window_width,
    window_aspect,
    snap_eps,
    cluster_eps,
    cluster_num_samples,
    floor_texture_scaling,
    wall_texture_scaling
  );

  g_logger.push_message({message, LogLevel::Text});
}

void Config::validate_config()
{
  if (unit_scale <= 0.0)
    throw std::runtime_error("unit_scale must be > 0");
  if (ceil_height <= 0.0)
    throw std::runtime_error("ceil_height must be > 0");
  if (door_width <= 0.0)
    throw std::runtime_error("door_width must be > 0");
  if (door_height <= 0.0)
    throw std::runtime_error("door_height must be > 0");
  if (window_sill_height < 0.0)
    throw std::runtime_error("window_sill_height must be >= 0");
  if (window_height <= 0.0)
    throw std::runtime_error("window_height must be > 0");
  if (window_width <= 0.0)
    throw std::runtime_error("window_width must be > 0");
  if (snap_eps <= 0.0)
    throw std::runtime_error("snap_eps must be > 0");
  if (cluster_eps <= 0.0)
    throw std::runtime_error("cluster_eps must be > 0");
  if (cluster_num_samples <= 0)
    throw std::runtime_error("cluster_num_samples must be > 0");
  if (floor_texture_scaling <= 0.0f)
    throw std::runtime_error("floor_texture_scaling must be > 0");
  if (wall_texture_scaling <= 0.0f)
    throw std::runtime_error("wall_texture_scaling must be > 0");

  if (door_width <= 0.0)
    throw std::runtime_error("door_width must be > 0");
  if (door_height <= 0.0)
    throw std::runtime_error("door_height must be > 0");
  if (door_height >= ceil_height)
      throw std::runtime_error("door_height must be < ceil_height");
    
  if (window_width <= 0.0)
    throw std::runtime_error("window_width must be > 0");
  if (window_height <= 0.0)
    throw std::runtime_error("window_height must be > 0");
  if (window_sill_height < 0.0)
    throw std::runtime_error("window_sill_height must be >= 0");
  if (window_sill_height >= ceil_height)
    throw std::runtime_error("window_sill_height must be < ceil_height");
}