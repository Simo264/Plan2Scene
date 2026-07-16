import bpy
import math
import os
import sys
import mathutils
import json

script_dir = os.path.dirname(os.path.abspath(__file__))
if script_dir not in sys.path:
  sys.path.insert(0, script_dir)

from render_utils import (
  clear_scene,
  setup_rendering_engine,
  import_gltf,
  place_openings,

  setup_world_hdri,
  apply_materials,
  setup_camera,
  setup_lights,
)

PROJECT_ROOT  = os.path.dirname(os.path.abspath(__file__))
CONFIG_JSON = os.path.join(PROJECT_ROOT, "blender_config.json")

# aspect ratio 16:9: (640x360), (960x540), (1280x720)

def main():
  # ==========================================
  # Load configuration from JSON file
  # ==========================================
  if not os.path.exists(CONFIG_JSON):
    print(f"Config file not found: {CONFIG_JSON}")
    return

  print(f"LOADING CONFIG: {CONFIG_JSON}")
  with open(CONFIG_JSON, 'r') as f:
    config = json.load(f)

  rendering = config["rendering"]
  scene_cfg = config["scene"]
  materials = config.get("materials", {})

  samples = rendering["samples"]
  res_x = rendering["resolution_x"]
  res_y = rendering["resolution_y"]
  output_path = rendering["output_image"]

  gltf_path = scene_cfg["gltf_path"]
  hdri_path = scene_cfg["hdri_path"]
  hdri_intensity = scene_cfg.get("hdri_intensity", 1.0)

  cam = scene_cfg["camera"]
  cam_loc = tuple(cam["location"])
  cam_rot_deg = cam["rotation_degrees"]
  cam_rot = (math.radians(cam_rot_deg[0]), math.radians(cam_rot_deg[1]), math.radians(cam_rot_deg[2]))
  cam_fov = cam["fov_degrees"]

  lights = scene_cfg["lights"]
  point_params = lights["point"]
  sun_params = lights["sun"]

  # blender setup
  
  clear_scene()
  
  setup_rendering_engine(samples, res_x, res_y, output_path)
  
  import_gltf(gltf_path)

  # ==========================================
  # Load and place openings (doors/windows)
  # ==========================================
  openings_path_rel = scene_cfg.get("opening_placeholders")
  openings_path = os.path.join(PROJECT_ROOT, openings_path_rel)
  door_asset_rel = scene_cfg.get("door_asset")
  door_asset_path = os.path.join(PROJECT_ROOT, door_asset_rel)
  window_asset_rel = scene_cfg.get("window_asset")
  window_asset_path = os.path.join(PROJECT_ROOT, window_asset_rel)

  if os.path.exists(openings_path):
    with open(openings_path, 'r') as f:
      openings_data = json.load(f)
    place_openings(openings_data["openings"], door_asset_path, window_asset_path)
  else:
    print(f"No openings file found: {openings_path}")

  setup_world_hdri(hdri_path, strength=hdri_intensity)

  apply_materials(materials)

  setup_camera(cam_loc, cam_rot, cam_fov)

  setup_lights(point_params, sun_params)

  # bpy.ops.render.render(write_still=True)
  
  scene_blend_path = os.path.join(PROJECT_ROOT, "out", "scene.blend")
  bpy.ops.wm.save_as_mainfile(filepath=scene_blend_path)

if __name__ == "__main__":
  main()