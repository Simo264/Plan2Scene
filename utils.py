import bpy, sys, math, os, json, mathutils
from pathlib import Path
from typing import List, Tuple, Any, Optional
from typeguard import typechecked

script_dir = os.path.dirname(os.path.abspath(__file__))
if script_dir not in sys.path:
  sys.path.insert(0, script_dir)

from models import BlenderConfig, OpeningInfo, Material
from material_helpers import apply_materials

@typechecked
def load_config(config_path: Path) -> BlenderConfig:
  try:
    with open(config_path, 'r') as f:
      raw_data = json.load(f)
    return BlenderConfig(**raw_data)
  except Exception as e:
    print(f"Error on opnerning file `{config_path}`: {e}")
    sys.exit(1)

@typechecked
def load_openings_data(openings_path: Path) -> list[OpeningInfo]:
  try:
    with open(openings_path, 'r') as f:
      raw_data = json.load(f)
  except Exception as e:
    print(f"Error on opening file `{openings_path}`: {e}")
    return []

  openings = []
  for idx, item in enumerate(raw_data["openings"]):
    try:
      opening = OpeningInfo(**item)
      openings.append(opening)
    except Exception as e:
      print(f"Validation error for aperture #{idx}: {e}")
      continue
  return openings

@typechecked
def save_blend_file(output_path: Path) -> None:
  output_path.parent.mkdir(parents=True, exist_ok=True)
  bpy.ops.wm.save_as_mainfile(filepath=str(output_path))

@typechecked
def setup_rendering_engine(
  samples: int,
  resolution_x: int,
  resolution_y: int,
  render_engine: str,
  use_denoising: bool
) -> None:

  print(f"samples={samples} ({type(samples).__name__})")
  print(f"resolution_x={resolution_x} ({type(resolution_x).__name__})")
  print(f"resolution_y={resolution_y} ({type(resolution_y).__name__})")
  print(f"render_engine={render_engine} ({type(render_engine).__name__})")
  print(f"use_denoising={use_denoising} ({type(use_denoising).__name__})")

  scene = bpy.context.scene
  scene.render.engine = render_engine
  scene.cycles.device = 'GPU'
  prefs = bpy.context.preferences.addons["cycles"].preferences
  prefs.refresh_devices()
  has_gpu = any(d.use for d in prefs.devices if d.type != 'CPU')
  if not has_gpu:
    print("No GPU found! Falling back to CPU rendering.")
    scene.cycles.device = 'CPU'
  
  scene.cycles.samples = samples
  scene.cycles.use_denoising = use_denoising
  
  scene.render.resolution_x = resolution_x
  scene.render.resolution_y = resolution_y

@typechecked
def import_model(
  model_path: Path,
  materials: List[Material],
  openings_data: list[OpeningInfo] = [],
  door_asset_path: Optional[Path] = None,
  window_asset_path: Optional[Path] = None,
) -> None:

  try:
    bpy.ops.import_scene.gltf(filepath=str(model_path))
  except Exception as e:
    print(f"Error on importing model `{model_path}`: {e}")
    sys.exit(1)

  # Apply materials
  apply_materials(materials[0], materials[1])

  # Handle openings
  if openings_data:
    _place_openings(openings_data, door_asset_path, window_asset_path)
  else:
    print("No opening data provided. No openings will be placed.")

@typechecked
def setup_hdr(
  hdri_path: Optional[Path], 
  strength: float = 1.0
) -> None:

  if hdri_path is None or not hdri_path.exists():
    print(f"Warning: HDR file not found or not specified: {hdri_path}. Skipping HDR setup.")
    return

  world = bpy.data.worlds.get('World')
  if world is None:
    world = bpy.data.worlds.new('World')
  bpy.context.scene.world = world

  world.use_nodes = True
  nodes = world.node_tree.nodes
  links = world.node_tree.links
  nodes.clear()

  node_texcoord = nodes.new(type='ShaderNodeTexCoord')
  node_mapping = nodes.new(type='ShaderNodeMapping')
  node_env_tex = nodes.new(type='ShaderNodeTexEnvironment')
  node_background = nodes.new(type='ShaderNodeBackground')
  node_output = nodes.new(type='ShaderNodeOutputWorld')

  node_output.location = (400, 0)
  node_background.location = (150, 0)
  node_env_tex.location = (-100, 0)
  node_mapping.location = (-350, 0)
  node_texcoord.location = (-550, 0)

  hdr_path_str = str(hdri_path.resolve())
  if hdr_path_str in bpy.data.images:
    node_env_tex.image = bpy.data.images[hdr_path_str]
  else:
    node_env_tex.image = bpy.data.images.load(hdr_path_str)

  links.new(node_texcoord.outputs['Generated'], node_mapping.inputs['Vector'])
  links.new(node_mapping.outputs['Vector'], node_env_tex.inputs['Vector'])
  links.new(node_env_tex.outputs['Color'], node_background.inputs['Color'])
  links.new(node_background.outputs['Background'], node_output.inputs['Surface'])
  node_background.inputs['Strength'].default_value = strength

@typechecked
def setup_camera(
  location: Tuple[float, float, float] = (0.0, 0.0, 0.0),
  rotation: Tuple[float, float, float] = (math.radians(90), 0.0, 0.0),
  fov_degrees: float = 45.0,
  camera_name: str = "Camera"
) -> None:
 
  if camera_name in bpy.data.objects:
    old_cam = bpy.data.objects[camera_name]
    bpy.data.objects.remove(old_cam, do_unlink=True)
  
  cam_data = bpy.data.cameras.new(camera_name)
  cam_data.lens_unit = 'FOV'
  cam_data.angle_y = math.radians(fov_degrees)
  
  cam_obj = bpy.data.objects.new(camera_name, cam_data)
  bpy.context.collection.objects.link(cam_obj)
  
  cam_obj.location = location
  cam_obj.rotation_euler = mathutils.Euler(rotation, 'XYZ')
  
  bpy.context.scene.camera = cam_obj
  bpy.context.view_layer.update()

@typechecked
def setup_lighting(
  point_energy: float = 700.0,
  point_color: Tuple[float, float, float] = (1.0, 0.95, 0.88),
  point_shadow_soft_size: float = 0.15,
  point_location: Tuple[float, float, float] = (0.0, 0.0, 0.5),
  
  area_energy: float = 200.0,
  area_color: Tuple[float, float, float] = (1.0, 1.0, 1.0),
  area_size: float = 2.0,
  area_location: Tuple[float, float, float] = (0.0, -3.0, 2.0),
  area_rotation: Tuple[float, float, float] = (math.radians(60), 0.0, 0.0),
  
  sun_energy: float = 2.5,
  sun_angle_degrees: float = 2.0,
  sun_rotation_degrees: Tuple[float, float, float] = (60.0, 0.0, 35.0)
) -> None:
    
  # Point light
  point_data = bpy.data.lights.new(name="IndoorLight", type='POINT')
  point_data.energy = point_energy
  point_data.color = point_color
  point_data.shadow_soft_size = point_shadow_soft_size
  point_obj = bpy.data.objects.new(name="IndoorLight", object_data=point_data)
  point_obj.location = point_location
  bpy.context.collection.objects.link(point_obj)

  # Area light
  area_data = bpy.data.lights.new(name="AreaLight", type='AREA')
  area_data.energy = area_energy
  area_data.color = area_color
  area_data.size = area_size
  area_obj = bpy.data.objects.new(name="AreaLight", object_data=area_data)
  area_obj.location = area_location
  area_obj.rotation_euler = mathutils.Euler(area_rotation, 'XYZ')
  bpy.context.collection.objects.link(area_obj)

  # Sun light
  sun_data = bpy.data.lights.new(name="SunLight", type='SUN')
  sun_data.energy = sun_energy
  sun_data.angle = math.radians(sun_angle_degrees)
  sun_obj = bpy.data.objects.new(name="SunLight", object_data=sun_data)
  sun_rot = tuple(math.radians(a) for a in sun_rotation_degrees)
  sun_obj.rotation_euler = mathutils.Euler(sun_rot, 'XYZ')
  bpy.context.collection.objects.link(sun_obj)




# ==========================================
# HELPER FUNCTIONS FOR OPENINGS
# ==========================================

@typechecked
def _place_openings(
  openings: List[OpeningInfo],
  door_glb_path: Optional[Path],
  window_glb_path: Optional[Path]
) -> None:

  for i, opening in enumerate(openings):
    op_type = opening.type
    loc, rot = _get_opening_transform_from_model(opening)
    width = opening.width
    height = opening.height
    thickness = opening.thickness

    if op_type == "Door":
      if door_glb_path and door_glb_path.exists():
        _place_glb_asset(door_glb_path, loc, rot, width, height, thickness, asset_name=f"Door_{i:03d}")
      else:
        _create_placeholder_door(loc, rot, width, height, thickness, name=f"Door_Placeholder_{i:03d}")

    elif op_type == "Window":
      if window_glb_path and window_glb_path.exists():
        _place_glb_asset(window_glb_path, loc, rot, width, height, thickness, asset_name=f"Window_{i:03d}")
      else:
        _create_placeholder_window(loc, rot, width, height, thickness, name=f"Window_Placeholder_{i:03d}")

@typechecked
def _get_opening_transform_from_model(opening: OpeningInfo) -> Tuple[Tuple[float, float, float], Tuple[float, float, float]]:
  gx, gy_height, gz_spatial = opening.center
  loc = (gx, -gz_spatial, gy_height)
  rot_z = -opening.rotation_z
  return loc, (0.0, 0.0, rot_z)

@typechecked
def _place_glb_asset(
  glb_path: Path,
  location: Tuple[float, float, float],
  rotation_euler: Tuple[float, float, float],
  target_width: float,
  target_height: float,
  target_depth: Optional[float] = None,
  asset_name: str = "asset"
) -> Optional[bpy.types.Object]:

  pre_import_objs = set(bpy.data.objects)

  try:
    bpy.ops.import_scene.gltf(filepath=str(glb_path))
  except Exception as e:
    print(f"Warning: failed on importing `{glb_path}`: {e}")
    return None

  imported = list(set(bpy.data.objects) - pre_import_objs)
  if not imported:
    print(f"Warning: no objects were imported from {glb_path}")
    return None

  roots = [o for o in imported if o.parent is None or o.parent not in imported]
  anchor = bpy.data.objects.new(asset_name, None)
  bpy.context.collection.objects.link(anchor)
  bpy.context.view_layer.update()

  bbox_corners = []
  for obj in imported:
    if obj.type == 'MESH':
      bbox_corners.extend([obj.matrix_world @ mathutils.Vector(c) for c in obj.bound_box])

  if not bbox_corners:
    print(f"Warning: no mesh found in `{glb_path}`")
    bpy.data.objects.remove(anchor)
    return None

  xs = [c.x for c in bbox_corners]
  ys = [c.y for c in bbox_corners]
  zs = [c.z for c in bbox_corners]
  asset_width = max(xs) - min(xs)
  asset_height = max(zs) - min(zs)
  asset_depth = max(ys) - min(ys)

  bbox_center = mathutils.Vector((
    (max(xs) + min(xs)) * 0.5,
    (max(ys) + min(ys)) * 0.5,
    (max(zs) + min(zs)) * 0.5,
  ))

  for obj in roots:
    obj.parent = anchor
    obj.matrix_parent_inverse = anchor.matrix_world.inverted()
    obj.location -= bbox_center

  EPS = 1e-6
  if target_depth is not None:
    scale_x = target_width / asset_width if asset_width > EPS else 1.0
    scale_y = target_depth / asset_depth if asset_depth > EPS else 1.0
    scale_z = target_height / asset_height if asset_height > EPS else 1.0
  else:
    uniform = target_width / asset_width if asset_width > EPS else 1.0
    scale_x = scale_y = scale_z = uniform

  anchor.location = location
  anchor.rotation_euler = rotation_euler
  anchor.scale = (abs(scale_x), abs(scale_y), abs(scale_z))
  return anchor

@typechecked
def _create_placeholder_door(
  location: Tuple[float, float, float],
  rotation: Tuple[float, float, float],
  width: float,
  height: float,
  thickness: float = 0.08,
  color: Tuple[float, float, float, float] = (0.4, 0.2, 0.1, 1.0),
  name: str = "Door_Placeholder"
) -> bpy.types.Object:

  bpy.ops.mesh.primitive_cube_add(size=1, location=location, rotation=rotation)
  obj = bpy.context.object
  obj.name = name
  obj.scale = (width, thickness, height)

  mat = bpy.data.materials.new(name=f"{name}_Material")
  mat.use_nodes = True
  principled = mat.node_tree.nodes["Principled BSDF"]
  principled.inputs["Base Color"].default_value = color

  if color[3] < 1.0:
    mat.blend_method = 'BLEND'
    principled.inputs["Alpha"].default_value = color[3]
  else:
    mat.blend_method = 'OPAQUE'

  obj.data.materials.append(mat)
  return obj

@typechecked
def _create_placeholder_window(
  location: Tuple[float, float, float],
  rotation: Tuple[float, float, float],
  width: float,
  height: float,
  thickness: float = 0.1,
  color: Tuple[float, float, float, float] = (0.2, 0.6, 1.0, 0.3),
  name: str = "Window_Placeholder"
) -> bpy.types.Object:

  bpy.ops.mesh.primitive_cube_add(size=1, location=location, rotation=rotation)
  obj = bpy.context.object
  obj.name = name
  obj.scale = (width, thickness, height)

  mat = bpy.data.materials.new(name=f"{name}_Material")
  mat.use_nodes = True
  principled = mat.node_tree.nodes["Principled BSDF"]
  principled.inputs["Base Color"].default_value = color
  mat.blend_method = 'BLEND'
  principled.inputs["Alpha"].default_value = color[3]

  obj.data.materials.append(mat)
  return obj
