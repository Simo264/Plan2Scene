import bpy
import math
import os
import json
import mathutils

PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__))

def clear_scene():
  bpy.ops.object.select_all(action='SELECT')
  bpy.ops.object.delete()

def setup_rendering_engine(samples, resolution_x, resolution_y, output_path):
  scene = bpy.context.scene
  scene.render.engine = 'CYCLES'
  scene.cycles.device = 'GPU'
  prefs = bpy.context.preferences.addons["cycles"].preferences
  prefs.refresh_devices()
  has_gpu = any(d.use for d in prefs.devices if d.type != 'CPU')
  if not has_gpu:
    print("No GPU found! Falling back to CPU rendering.")
    scene.cycles.device = 'CPU'
  
  scene.cycles.samples = samples
  scene.cycles.use_denoising = True
  
  scene.render.resolution_x = resolution_x
  scene.render.resolution_y = resolution_y
  scene.render.filepath = output_path

  print(f"RENDER ENGINE: {scene.cycles.device}, samples={samples}, resolution={resolution_x}x{resolution_y}")
  print(f"OUTPUT IMAGE : {os.path.abspath(output_path)}")

def import_gltf(filepath):
  if not os.path.isabs(filepath):
    filepath = os.path.join(PROJECT_ROOT, filepath)
  abs_path = os.path.abspath(filepath)
  if not os.path.exists(abs_path):
    print(f"GLTF file not found: {abs_path}")
    return
  print(f"IMPORT GLTF  : {abs_path}")
  bpy.ops.import_scene.gltf(filepath=abs_path)

def place_openings(openings, door_glb_path, window_glb_path=None):
  for i, opening in enumerate(openings):
    op_type = opening.get("type")
    loc, rot = _get_opening_transform(opening)
    width = opening["width"]
    height = opening["height"]
    thickness = opening.get("thickness", 0.1)

    if op_type == "Door":
      if door_glb_path and os.path.exists(door_glb_path):
        _place_glb_asset(door_glb_path, loc, rot, width, height, thickness, asset_name=f"Door_{i:03d}")
      else:
        _create_placeholder_door(loc, rot, width, height, thickness, name=f"Door_Placeholder_{i:03d}")

    elif op_type == "Window":
      if window_glb_path and os.path.exists(window_glb_path):
        _place_glb_asset(window_glb_path, loc, rot, width, height, thickness, asset_name=f"Window_{i:03d}")
      else:
        _create_placeholder_window(loc, rot, width, height, thickness, name=f"Window_Placeholder_{i:03d}")

def setup_camera(location, rotation, fov_degrees):
  cam_data = bpy.data.cameras.new('MainCamera')
  cam_data.lens_unit = 'FOV'
  cam_data.angle_y = math.radians(fov_degrees)
  cam_obj = bpy.data.objects.new('MainCamera', cam_data)
  bpy.context.collection.objects.link(cam_obj)
  bpy.context.scene.camera = cam_obj
  cam_obj.location = location
  cam_obj.rotation_euler = mathutils.Euler(rotation, 'XYZ')
  bpy.context.view_layer.update()

def setup_lights(point_params, sun_params):
  # Point light
  light_data = bpy.data.lights.new(name="IndoorLight", type='POINT')
  light_data.energy = point_params.get("energy", 700.0)
  light_data.color = tuple(point_params.get("color", [1.0, 0.95, 0.88]))
  light_data.shadow_soft_size = point_params.get("shadow_soft_size", 0.15)
  light_obj = bpy.data.objects.new(name="IndoorLight", object_data=light_data)
  light_obj.location = tuple(point_params.get("location", [0.0, 0.0, 0.5]))
  bpy.context.collection.objects.link(light_obj)

  # Sun light
  sun_data = bpy.data.lights.new(name="KeyLight", type='SUN')
  sun_data.energy = sun_params.get("energy", 2.5)
  sun_data.angle = math.radians(sun_params.get("angle_degrees", 2.0))
  sun_obj = bpy.data.objects.new(name="KeyLight", object_data=sun_data)
  rot = sun_params.get("rotation_degrees", [60.0, 0.0, 35.0])
  sun_obj.rotation_euler = (math.radians(rot[0]), math.radians(rot[1]), math.radians(rot[2]))
  bpy.context.collection.objects.link(sun_obj)

def setup_world_hdri(hdri_path, strength=1.0):
  # If path is relative, resolve it
  if not os.path.isabs(hdri_path):
    hdri_path = os.path.join(PROJECT_ROOT, hdri_path)
  abs_path = os.path.abspath(hdri_path)
  if not os.path.exists(abs_path):
    print(f"Warning: HDRI not found {abs_path}")
    return

  print(f"HDRI IMAGE   : {abs_path} (intensity={strength})")
  world = bpy.data.worlds.get("World")
  if world is None:
    world = bpy.data.worlds.new("World")
  bpy.context.scene.world = world

  world.use_nodes = True
  nodes = world.node_tree.nodes
  links = world.node_tree.links
  nodes.clear()

  node_output = nodes.new("ShaderNodeOutputWorld")
  node_background = nodes.new("ShaderNodeBackground")
  node_env_tex = nodes.new("ShaderNodeTexEnvironment")
  node_mapping = nodes.new("ShaderNodeMapping")
  node_texcoord = nodes.new("ShaderNodeTexCoord")

  node_output.location = (400, 0)
  node_background.location = (150, 0)
  node_env_tex.location = (-100, 0)
  node_mapping.location = (-350, 0)
  node_texcoord.location = (-550, 0)

  # Load HDR image
  img_name = os.path.basename(abs_path)
  if img_name in bpy.data.images:
    node_env_tex.image = bpy.data.images[img_name]
  else:
    node_env_tex.image = bpy.data.images.load(abs_path)

  # HDRIs are loaded as 'Non-Color' by default in Blender
  node_env_tex.image.colorspace_settings.name = 'Non-Color'

  links.new(node_texcoord.outputs["Generated"], node_mapping.inputs["Vector"])
  links.new(node_mapping.outputs["Vector"], node_env_tex.inputs["Vector"])
  links.new(node_env_tex.outputs["Color"], node_background.inputs["Color"])
  links.new(node_background.outputs["Background"], node_output.inputs["Surface"])

  # Ambient light intensity
  node_background.inputs["Strength"].default_value = strength

def apply_materials(materials_dict):
  for material in bpy.data.materials:
    base_name = material.name.split('.')[0]
    if base_name in materials_dict:
      print(f"Applying material: `{base_name}`")
      setup_material_nodes(material, materials_dict[base_name])
    else:
      print(f"Material not found `{base_name}`. Using default.")

def load_image_node(nodes, imagepath, colorspace='sRGB'):
  if not imagepath:
    return None

  # If the path is relative, resolve it against PROJECT_ROOT
  if not os.path.isabs(imagepath):
    imagepath = os.path.join(PROJECT_ROOT, imagepath)
  abs_path = os.path.abspath(imagepath)
  if not os.path.exists(abs_path):
    print(f"Warning: image not found {abs_path}")
    return None
  node_tex = nodes.new("ShaderNodeTexImage")
  img_name = os.path.basename(abs_path)
  if img_name in bpy.data.images:
    node_tex.image = bpy.data.images[img_name]
  else:
    node_tex.image = bpy.data.images.load(abs_path)
  
  node_tex.image.colorspace_settings.name = colorspace
  return node_tex

def setup_material_nodes(mat, mat_config):
  mat_name = mat.name
  print(f"  Configuring material: '{mat_name}'")
  nodes, links, node_principled, node_output, node_mapping = _setup_base_nodes(mat)

  albedo_output = _handle_albedo(nodes, links, node_mapping, node_principled, mat_config)
  ao_output, roughness_output, metallic_output = _handle_arm_or_single(nodes, links, node_mapping, node_principled, mat_config)

  _mix_albedo_ao(nodes, links, albedo_output, ao_output, node_principled, mat_config)
  _handle_normal(nodes, links, node_mapping, node_principled, mat_config)
  _handle_displacement(nodes, links, node_mapping, node_output, mat, mat_config)


# ==========================================
# HELPER FUNCTIONS FOR MATERIALS MANAGEMENT
# ==========================================

def _setup_base_nodes(mat):
  mat.use_nodes = True
  nodes = mat.node_tree.nodes
  links = mat.node_tree.links
  nodes.clear()

  node_output = nodes.new("ShaderNodeOutputMaterial")
  node_principled = nodes.new("ShaderNodeBsdfPrincipled")
  node_mapping = nodes.new("ShaderNodeMapping")
  node_texcoord = nodes.new("ShaderNodeTexCoord")

  node_output.location = (400, 0)
  node_principled.location = (50, 0)
  node_mapping.location = (-900, 0)
  node_texcoord.location = (-1100, 0)

  links.new(node_texcoord.outputs["UV"], node_mapping.inputs["Vector"])
  links.new(node_principled.outputs["BSDF"], node_output.inputs["Surface"])

  return nodes, links, node_principled, node_output, node_mapping

def _handle_albedo(nodes, links, node_mapping, node_principled, mat_config):
  albedo_path = mat_config.get("albedo")
  base_color = mat_config.get("base_color", [1.0, 1.0, 1.0])
  if albedo_path:
    print(f"    - Albedo texture: {albedo_path}")
    tex_albedo = load_image_node(nodes, albedo_path, 'sRGB')
    if tex_albedo:
      tex_albedo.location = (-600, 300)
      links.new(node_mapping.outputs["Vector"], tex_albedo.inputs["Vector"])
      return tex_albedo.outputs["Color"]
    else:
      print(f"    - Albedo texture NOT FOUND, fallback to base color {base_color}")
      node_principled.inputs["Base Color"].default_value = (*base_color, 1.0)
      return None
  else:
    print(f"    - No albedo texture, using base color {base_color}")
    node_principled.inputs["Base Color"].default_value = (*base_color, 1.0)
    return None

def _handle_arm_or_single(nodes, links, node_mapping, node_principled, mat_config):
  arm_path = mat_config.get("arm")
  ao_path = mat_config.get("ao")
  roughness_path = mat_config.get("roughness")
  metallic_path = mat_config.get("metallic")

  roughness_val = mat_config.get("roughness_value", 0.5)
  metallic_val = mat_config.get("metallic_value", 0.0)

  ao_output = None
  roughness_output = None
  metallic_output = None

  if arm_path:
    print(f"    - Using ARM texture: {arm_path}")
    tex_arm = load_image_node(nodes, arm_path, 'Non-Color')
    if tex_arm:
      tex_arm.location = (-600, 0)
      links.new(node_mapping.outputs["Vector"], tex_arm.inputs["Vector"])
      node_sep = nodes.new("ShaderNodeSeparateColor")
      node_sep.location = (-350, -50)
      links.new(tex_arm.outputs["Color"], node_sep.inputs["Color"])
      ao_output = node_sep.outputs["Red"]
      roughness_output = node_sep.outputs["Green"]
      metallic_output = node_sep.outputs["Blue"]
      print("      - ARM: AO=R, Roughness=G, Metallic=B")
    else:
      print("      - ARM texture NOT FOUND, fallback to individual channels/values")
      arm_path = None 

  if not arm_path:
    # Ambient occlusion - AO
    if ao_path:
      print(f"    - AO texture: {ao_path}")
      tex_ao = load_image_node(nodes, ao_path, 'Non-Color')
      if tex_ao:
        tex_ao.location = (-600, 150)
        links.new(node_mapping.outputs["Vector"], tex_ao.inputs["Vector"])
        ao_output = tex_ao.outputs["Color"]
      else:
        print("      - AO texture NOT FOUND, no AO applied")
    else:
      print("    - No AO texture, no AO applied")

    # Roughness
    if roughness_path:
      print(f"    - Roughness texture: {roughness_path}")
      tex_rough = load_image_node(nodes, roughness_path, 'Non-Color')
      if tex_rough:
        tex_rough.location = (-600, -150)
        links.new(node_mapping.outputs["Vector"], tex_rough.inputs["Vector"])
        roughness_output = tex_rough.outputs["Color"]
      else:
        print(f"      - Roughness texture NOT FOUND, using value {roughness_val}")
        node_principled.inputs["Roughness"].default_value = roughness_val
    else:
      print(f"    - No roughness texture, using value {roughness_val}")
      node_principled.inputs["Roughness"].default_value = roughness_val

    # Metallic
    if metallic_path:
      print(f"    - Metallic texture: {metallic_path}")
      tex_metallic = load_image_node(nodes, metallic_path, 'Non-Color')
      if tex_metallic:
        tex_metallic.location = (-600, -300)
        links.new(node_mapping.outputs["Vector"], tex_metallic.inputs["Vector"])
        metallic_output = tex_metallic.outputs["Color"]
      else:
        print(f"      - Metallic texture NOT FOUND, using value {metallic_val}")
        node_principled.inputs["Metallic"].default_value = metallic_val
    else:
      print(f"    - No metallic texture, using value {metallic_val}")
      node_principled.inputs["Metallic"].default_value = metallic_val

  if roughness_output:
    links.new(roughness_output, node_principled.inputs["Roughness"])
  if metallic_output:
    links.new(metallic_output, node_principled.inputs["Metallic"])

  return ao_output, roughness_output, metallic_output

def _mix_albedo_ao(nodes, links, albedo_output, ao_output, node_principled, mat_config):
  if albedo_output is not None and ao_output is not None:
    ao_mix_factor = mat_config.get("ao_mix_factor", 1.0)
    print(f"    - Mixing Albedo with AO (factor={ao_mix_factor})")
    node_mix = nodes.new("ShaderNodeMixRGB")
    node_mix.blend_type = 'MULTIPLY'
    node_mix.inputs["Fac"].default_value = ao_mix_factor
    node_mix.location = (-250, 200)
    links.new(albedo_output, node_mix.inputs["Color1"])
    links.new(ao_output, node_mix.inputs["Color2"])
    links.new(node_mix.outputs["Color"], node_principled.inputs["Base Color"])
  elif albedo_output is not None:
    print("    - No AO to mix, using Albedo directly")
    links.new(albedo_output, node_principled.inputs["Base Color"])
  else:
    print("    - No Albedo, base color already set")

def _handle_normal(nodes, links, node_mapping, node_principled, mat_config):
  normal_path = mat_config.get("normal")
  normal_strength = mat_config.get("normal_strength", 1.5)
  if normal_path:
    print(f"    - Normal map: {normal_path} (strength={normal_strength})")
    tex_normal = load_image_node(nodes, normal_path, 'Non-Color')
    if tex_normal:
      tex_normal.location = (-600, -400)
      node_normal_map = nodes.new("ShaderNodeNormalMap")
      node_normal_map.location = (-250, -400)
      node_normal_map.inputs["Strength"].default_value = normal_strength
      links.new(node_mapping.outputs["Vector"], tex_normal.inputs["Vector"])
      links.new(tex_normal.outputs["Color"], node_normal_map.inputs["Color"])
      links.new(node_normal_map.outputs["Normal"], node_principled.inputs["Normal"])
    else:
      print("      - Normal map texture NOT FOUND, skipping")
  else:
    print("    - No normal map")

def _handle_displacement(nodes, links, node_mapping, node_output, mat, mat_config):
  disp_path = mat_config.get("displacement")
  disp_scale = mat_config.get("disp_scale", 0.05)
  if disp_path:
    print(f"    - Displacement map: {disp_path} (scale={disp_scale})")
    tex_disp = load_image_node(nodes, disp_path, 'Non-Color')
    if tex_disp:
      tex_disp.location = (-600, -650)
      node_disp = nodes.new("ShaderNodeDisplacement")
      node_disp.location = (-250, -650)
      node_disp.inputs["Scale"].default_value = disp_scale
      node_disp.inputs["Midlevel"].default_value = 0.5
      links.new(node_mapping.outputs["Vector"], tex_disp.inputs["Vector"])
      links.new(tex_disp.outputs["Color"], node_disp.inputs["Height"])
      links.new(node_disp.outputs["Displacement"], node_output.inputs["Displacement"])
      
      if hasattr(mat.cycles, 'displacement_method'):
        mat.cycles.displacement_method = 'BOTH'
        print("      - Displacement method set to 'BOTH'")
      else:
        print("      - Displacement_method not available, using default")
    else:
      print("      - Displacement texture NOT FOUND, skipping")
  else:
    print("    - No displacement map")

# ==========================================
# HELPER FUNCTIONS FOR OPENINGS
# ==========================================


def _create_placeholder_door(location, rotation, width, height, thickness=0.08, color=(0.4, 0.2, 0.1, 1.0), name="Door_Placeholder"):
  bpy.ops.mesh.primitive_cube_add(size=1, location=location, rotation=rotation)
  obj = bpy.context.object
  obj.name = name
  obj.scale = (width, thickness, height)
  mat = bpy.data.materials.new(name=f"{name}_Material")
  mat.use_nodes = True
  mat.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = color
  if color[3] < 1.0:
    mat.blend_method = 'BLEND'
    mat.node_tree.nodes["Principled BSDF"].inputs["Alpha"].default_value = color[3]
  else:
    mat.blend_method = 'OPAQUE'
  obj.data.materials.append(mat)
  return obj

def _create_placeholder_window(location, rotation, width, height, thickness=0.1, color=(0.2, 0.6, 1.0, 0.3), name="Window_Placeholder"):
  bpy.ops.mesh.primitive_cube_add(size=1, location=location, rotation=rotation)
  obj = bpy.context.object
  obj.name = name
  obj.scale = (width, thickness, height)
  mat = bpy.data.materials.new(name=f"{name}_Material")
  mat.use_nodes = True
  mat.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = color
  mat.blend_method = 'BLEND'
  mat.node_tree.nodes["Principled BSDF"].inputs["Alpha"].default_value = color[3]
  obj.data.materials.append(mat)
  return obj

def _get_opening_transform(opening):
  gx, gy_height, gz_spatial = opening["center"]
  loc = (gx, -gz_spatial, gy_height)
  rot_z = -opening["rotation_z"]
  return loc, (0.0, 0.0, rot_z)

def _place_glb_asset(glb_path, location, rotation_euler, target_width, target_height, target_depth=None, asset_name="asset"):
  """
  Importa un file GLB, lo posiziona e lo scala per adattarsi alle dimensioni target.
  - Se target_depth è None, scala uniformemente (larghezza come riferimento).
  - asset_name: nome dell'oggetto anchor.
  - Restituisce l'oggetto anchor o None in caso di fallimento.
  """
  pre_import_objs = set(bpy.data.objects)
  try:
      bpy.ops.import_scene.gltf(filepath=glb_path)
  except Exception as e:
      print(f"Import fallito per {glb_path}: {e}")
      return None

  imported = list(set(bpy.data.objects) - pre_import_objs)
  if not imported:
      print(f"Nessun oggetto importato da {glb_path}")
      return None

  roots = [o for o in imported if o.parent is None or o.parent not in imported]

  # Crea un anchor
  anchor = bpy.data.objects.new(asset_name, None)
  bpy.context.collection.objects.link(anchor)
  bpy.context.view_layer.update()

  # Calcola bounding box di tutte le mesh importate
  bbox_corners = []
  for obj in imported:
      if obj.type == 'MESH':
          bbox_corners.extend([obj.matrix_world @ mathutils.Vector(c) for c in obj.bound_box])

  if not bbox_corners:
      print(f"Nessuna mesh trovata in {glb_path}")
      bpy.data.objects.remove(anchor)
      return None

  xs = [c.x for c in bbox_corners]
  ys = [c.y for c in bbox_corners]
  zs = [c.z for c in bbox_corners]
  asset_width  = max(xs) - min(xs)
  asset_height = max(zs) - min(zs)
  asset_depth  = max(ys) - min(ys)

  bbox_center = mathutils.Vector((
      (max(xs) + min(xs)) * 0.5,
      (max(ys) + min(ys)) * 0.5,
      (max(zs) + min(zs)) * 0.5,
  ))

  # Rende i figli figli dell'anchor e centra la bbox
  for obj in roots:
      obj.parent = anchor
      obj.matrix_parent_inverse = anchor.matrix_world.inverted()
      obj.location -= bbox_center

  # Calcola scale (positive)
  scale_x = target_width / asset_width if asset_width > 1e-6 else 1.0
  scale_z = target_height / asset_height if asset_height > 1e-6 else 1.0

  if target_depth is not None:
      scale_y = target_depth / asset_depth if asset_depth > 1e-6 else 1.0
  else:
      # Scala uniforme: usa la larghezza come riferimento
      uniform = target_width / asset_width if asset_width > 1e-6 else 1.0
      scale_x = scale_y = scale_z = uniform

  anchor.location = location
  anchor.rotation_euler = rotation_euler
  anchor.scale = (abs(scale_x), abs(scale_y), abs(scale_z))   # Forza positivo

  return anchor

