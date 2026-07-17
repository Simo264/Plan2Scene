import json
import sys
import math
import bpy
import mathutils
import os
from pathlib import Path

script_dir = os.path.dirname(os.path.abspath(__file__))
if script_dir not in sys.path:
  sys.path.insert(0, script_dir)

from models import Material, BlenderConfig, OpeningInfo
from utils import (
  load_config,
  load_openings_data,
  save_blend_file,

  setup_rendering_engine,
  import_model,

  setup_hdr,
  setup_camera,
  setup_lighting,
)

# aspect ratio 16:9: (640x360), (960x540), (1280x720)

def main():
  config = load_config(Path("blender_config.json"))

  # clear scene
  bpy.ops.object.select_all(action='SELECT')
  bpy.ops.object.delete()

  # setup rendering engine 
  setup_rendering_engine(
    samples=config.samples,
    resolution_x=config.resolution_x,
    resolution_y=config.resolution_y,
    render_engine=config.render_engine,
    use_denoising=config.use_denoising
  )

  # setup hdr image
  setup_hdr(hdri_path=config.hdri_path, strength=config.hdri_intensity)

  # camera setup
  setup_camera(
    location=(0.0, -5.0, 2.0),
    rotation=(math.radians(60), 0.0, 0.0),
    fov_degrees=45.0
  )

  # import the model
  materials = [config.floor_material, config.wall_and_ceil_material]
  openings_list = load_openings_data(config.opening_placeholders)
  import_model(
    model_path=config.model_path,
    materials=materials,
    openings_data=openings_list,
    door_asset_path=config.door_asset,
    window_asset_path=config.window_asset,
  )

  # setup lighting
  setup_lighting()

  # save blender file
  save_blend_file(config.output_blender)

if __name__ == "__main__":
  main()