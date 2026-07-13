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


def place_door_model(opening, glb_path, index=0):
  gx, gy_height, gz_spatial = opening["center"]
  blender_loc = (gx, -gz_spatial, gy_height)
  blender_rot_z = -opening["rotation_z"]

  pre_import_objs = set(bpy.data.objects)
  bpy.ops.import_scene.gltf(filepath=glb_path)
  imported = list(set(bpy.data.objects) - pre_import_objs)
  if not imported:
    print(f"Import failed for {glb_path}")
    return

  roots = [o for o in imported if o.parent is None or o.parent not in imported]

  anchor = bpy.data.objects.new(f"door_anchor_{index}", None)
  bpy.context.collection.objects.link(anchor)

  bpy.context.view_layer.update()

  # bbox in world space, cosi' come importato (anchor ancora a origine (0,0,0))
  bbox_corners = []
  for obj in imported:
    if obj.type == 'MESH':
      bbox_corners.extend([obj.matrix_world @ mathutils.Vector(c) for c in obj.bound_box])

  if not bbox_corners:
    print(f"No mesh data found in {glb_path}")
    return

  xs = [c.x for c in bbox_corners]
  ys = [c.y for c in bbox_corners]
  zs = [c.z for c in bbox_corners]
  asset_width  = max(xs) - min(xs)
  asset_height = max(zs) - min(zs)
  asset_depth  = max(ys) - min(ys)

  # centro reale della bbox (nel sistema mondo, con anchor ancora a (0,0,0))
  bbox_center = mathutils.Vector((
    (max(xs) + min(xs)) * 0.5,
    (max(ys) + min(ys)) * 0.5,
    (max(zs) + min(zs)) * 0.5,
  ))

  # ora parentiamo, MA compensando l'offset tra origin (0,0,0) e bbox_center:
  # ogni root viene traslato dell'opposto del centro bbox, cosi' che il centro
  # geometrico della porta coincida con l'origine dell'anchor
  for obj in roots:
    obj.parent = anchor
    obj.matrix_parent_inverse = anchor.matrix_world.inverted()
    obj.location -= bbox_center

  scale_x = opening["width"] / asset_width if asset_width > 1e-6 else 1.0
  scale_y = opening["thickness"] / asset_depth if asset_depth > 1e-6 else 1.0
  scale_z = opening["height"] / asset_height if asset_height > 1e-6 else 1.0

  anchor.location = blender_loc
  anchor.rotation_euler = (0.0, 0.0, blender_rot_z)
  # anchor.scale = (scale_x, scale_y, scale_z)

def place_openings(openings, glb_path):
  for i, opening in enumerate(openings):
    if opening["type"] == "Door":
      place_door_model(opening, glb_path, index=i)



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
  openings_path = "out/draftperson_Floor_Plan_openings.json"
  door_glb_path = os.path.join(PROJECT_ROOT, "assets/door_classic.glb")
  if os.path.exists(openings_path):
    with open(openings_path, 'r') as f:
      openings_data = json.load(f)
    place_openings(openings_data["openings"], door_glb_path)
  else:
    print(f"No openings file found at {openings_path}, skipping")

  setup_world_hdri(hdri_path, strength=hdri_intensity)

  apply_materials(materials)

  setup_camera(cam_loc, cam_rot, cam_fov)

  setup_lights(point_params, sun_params)

  # bpy.ops.render.render(write_still=True)
  
  scene_blend_path = os.path.join(PROJECT_ROOT, "out", "scene.blend")
  bpy.ops.wm.save_as_mainfile(filepath=scene_blend_path)

if __name__ == "__main__":
  main()