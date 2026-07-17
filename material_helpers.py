# ==========================================
# HELPER FUNCTIONS FOR MATERIALS MANAGEMENT
# ==========================================

import bpy, sys, os
from pathlib import Path
from typing import Tuple, Optional
from typeguard import typechecked

script_dir = os.path.dirname(os.path.abspath(__file__))
if script_dir not in sys.path:
  sys.path.insert(0, script_dir)

from models import Material

@typechecked
def apply_materials(
  floor_material: Material,
  wall_ceil_material: Material
) -> None:

  if not bpy.data.materials:
      print("Warning: no material found in the scene.")
      return
  
  for mat in bpy.data.materials:
    base_name = mat.name.split('.')[0].lower()
    
    if 'floor' in base_name:
      data = floor_material
      print(f"Apply floor material `{mat.name}`")
    elif 'wall' in base_name or 'ceil' in base_name:
      data = wall_ceil_material
      print(f"Apply wall/ceil  material `{mat.name}`")
    else:
      print(f"Warning: unkown material found `{mat.name}` (`{base_name}`)")
      continue
    
    _setup_material_nodes(mat, data)



@typechecked
def _setup_material_nodes(
  mat: bpy.types.Material, 
  mat_config: Material
) -> None:

  mat_name = mat.name
  print(f"  Configuring material: '{mat_name}'")

  nodes, links, node_principled, node_output, node_mapping = _setup_base_nodes(mat)

  albedo_output = _handle_albedo(nodes, links, node_mapping, node_principled, mat_config)
  ao_output, roughness_output, metallic_output = _handle_arm_or_single(nodes, links, node_mapping, node_principled, mat_config)

  _mix_albedo_ao(nodes, links, albedo_output, ao_output, node_principled, mat_config)
  _handle_normal(nodes, links, node_mapping, node_principled, mat_config)
  _handle_displacement(nodes, links, node_mapping, node_output, mat, mat_config)

@typechecked
def _load_image_node(
  nodes, 
  image_path: Optional[Path], 
  color_space: str = 'sRGB'
) -> Optional[bpy.types.ShaderNodeTexImage]:

  if image_path is None or not image_path.exists():
    return None

  path_str = str(image_path.resolve())
  if path_str in bpy.data.images:
    img = bpy.data.images[path_str]
  else:
    try:
      img = bpy.data.images.load(path_str)
    except Exception as e:
      print(f"      Error on loading image `{path_str}`: {e}")
      return None

  try:
    img.colorspace_settings.name = color_space
  except AttributeError:
    pass

  node_tex = nodes.new(type='ShaderNodeTexImage')
  node_tex.image = img
  return node_tex

@typechecked
def _setup_base_nodes(mat: bpy.types.Material):
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

@typechecked
def _handle_albedo(
  nodes,
  links,
  node_mapping,
  node_principled,
  mat_config: Material
) -> Optional[bpy.types.NodeSocket]:

  base_color = mat_config.base_color
  albedo_path = mat_config.albedo
  if albedo_path and albedo_path.exists():
    print(f"    - Albedo texture: {albedo_path}")
    tex_albedo = _load_image_node(nodes, albedo_path, 'sRGB')
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

@typechecked
def _handle_arm_or_single(
  nodes,
  links,
  node_mapping,
  node_principled,
  mat_config: Material
) -> Tuple[Optional[bpy.types.NodeSocket], Optional[bpy.types.NodeSocket], Optional[bpy.types.NodeSocket]]:

  arm_path = mat_config.arm
  ao_path = mat_config.ao
  roughness_path = mat_config.roughness
  metallic_path = mat_config.metallic

  roughness_val = mat_config.roughness_value
  metallic_val = mat_config.metallic_value

  ao_output = None
  roughness_output = None
  metallic_output = None

  # ARM
  if arm_path and arm_path.exists():
    print(f"    - Using ARM texture: {arm_path}")
    tex_arm = _load_image_node(nodes, arm_path, 'Non-Color')
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

  # AO
  if not arm_path:
    if ao_path and ao_path.exists():
      print(f"    - AO texture: {ao_path}")
      tex_ao = _load_image_node(nodes, ao_path, 'Non-Color')
      if tex_ao:
        tex_ao.location = (-600, 150)
        links.new(node_mapping.outputs["Vector"], tex_ao.inputs["Vector"])
        ao_output = tex_ao.outputs["Color"]
      else:
        print("      - AO texture NOT FOUND, no AO applied")
    else:
      print("    - No AO texture, no AO applied")

    # Roughness
    if roughness_path and roughness_path.exists():
      print(f"    - Roughness texture: {roughness_path}")
      tex_rough = _load_image_node(nodes, roughness_path, 'Non-Color')
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
    if metallic_path and metallic_path.exists():
      print(f"    - Metallic texture: {metallic_path}")
      tex_metallic = _load_image_node(nodes, metallic_path, 'Non-Color')
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

@typechecked
def _mix_albedo_ao(
  nodes,
  links,
  albedo_output: Optional[bpy.types.NodeSocket],
  ao_output: Optional[bpy.types.NodeSocket],
  node_principled,
  mat_config: Material
) -> None:

  if albedo_output is not None and ao_output is not None:
    ao_mix_factor = mat_config.ao_mix_factor
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

@typechecked
def _handle_normal(
  nodes,
  links,
  node_mapping,
  node_principled,
  mat_config: Material
) -> None:

  normal_path = mat_config.normal
  normal_strength = mat_config.normal_strength
  if normal_path and normal_path.exists():
    print(f"    - Normal map: {normal_path} (strength={normal_strength})")
    tex_normal = _load_image_node(nodes, normal_path, 'Non-Color')
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

@typechecked
def _handle_displacement(
  nodes,
  links,
  node_mapping,
  node_output,
  mat: bpy.types.Material,
  mat_config: Material
) -> None:

  disp_path = mat_config.displacement
  disp_scale = mat_config.disp_scale
  if disp_path and disp_path.exists():
    print(f"    - Displacement map: {disp_path} (scale={disp_scale})")
    tex_disp = _load_image_node(nodes, disp_path, 'Non-Color')
    if tex_disp:
      tex_disp.location = (-600, -650)
      node_disp = nodes.new("ShaderNodeDisplacement")
      node_disp.location = (-250, -650)
      node_disp.inputs["Scale"].default_value = disp_scale
      node_disp.inputs["Midlevel"].default_value = 0.5
      links.new(node_mapping.outputs["Vector"], tex_disp.inputs["Vector"])
      links.new(tex_disp.outputs["Color"], node_disp.inputs["Height"])
      links.new(node_disp.outputs["Displacement"], node_output.inputs["Displacement"])

      # Imposta il metodo di displacement per Cycles
      if hasattr(mat.cycles, 'displacement_method'):
        mat.cycles.displacement_method = 'BOTH'
        print("      - Displacement method set to 'BOTH'")
      else:
        print("      - Displacement_method not available, using default")
    else:
      print("      - Displacement texture NOT FOUND, skipping")
  else:
    print("    - No displacement map")

