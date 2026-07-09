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
  setup_world_hdri,
  apply_materials,
  setup_camera,
  setup_lights,
)

PROJECT_ROOT  = os.path.dirname(os.path.abspath(__file__))
CONFIG_JSON = os.path.join(PROJECT_ROOT, "blender_config.json")

# aspect ratio 16:9
# - 640 x 360
# - 960 x 540
# - 1280 x 720
# IMAGE_RESOLUTION_W = 640;
# IMAGE_RESOLUTION_H = 360;

def main():
  # ==========================================
  # Load configuration from JSON file
  # ==========================================
  if not os.path.exists(CONFIG_JSON):
    print(f"Config file not found: {CONFIG_JSON}")
    return

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

  # setup blender
  clear_scene()
  
  setup_rendering_engine(samples, res_x, res_y, output_path)
  
  import_gltf(gltf_path)

  setup_world_hdri(hdri_path, strength=hdri_intensity)

  apply_materials(materials)

  setup_camera(cam_loc, cam_rot, cam_fov)

  setup_lights(point_params, sun_params)

  # Start rendering
  bpy.ops.render.render(write_still=True)

if __name__ == "__main__":
  main()