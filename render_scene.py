import bpy, math, os, mathutils, json

GLTF_PATH = "/home/simone/Desktop/Plan2Scene/tmp/Simple_House_Plan.gltf"
CONFIG_PATH = "/home/simone/Desktop/Plan2Scene/materials.json"
OUTPUT_IMAGE = "/home/simone/Desktop/Plan2Scene/tmp/output.png"

# Texture diffuse maps
FLOOR_TEX_DIFF = "/home/simone/Desktop/Plan2Scene/materials/interior_tiles/interior_tiles_diff_1k.jpg"
WALL_TEX_DIFF = "/home/simone/Desktop/Plan2Scene/materials/concrete_layers/concrete_layers_diff_1k.jpg"

# Texture normal maps
FLOOR_TEX_NORM = "/home/simone/Desktop/Plan2Scene/materials/interior_tiles/interior_tiles_nor_gl_1k.jpg"
WALL_TEX_NORM = "/home/simone/Desktop/Plan2Scene/materials/concrete_layers/concrete_layers_nor_gl_1k.jpg"

def load_image_node(nodes, filepath, colorspace='sRGB'):
  if not filepath:
    return None
      
  abs_path = os.path.abspath(filepath)
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
  mat.use_nodes = True
  nodes = mat.node_tree.nodes
  links = mat.node_tree.links
  nodes.clear()

  # Nodi Base
  node_output = nodes.new("ShaderNodeOutputMaterial")
  node_principled = nodes.new("ShaderNodeBsdfPrincipled")
  node_mapping = nodes.new("ShaderNodeMapping")
  node_texcoord = nodes.new("ShaderNodeTexCoord")

  node_output.location = (400, 0)
  node_principled.location = (50, 0)
  node_mapping.location = (-600, 0)
  node_texcoord.location = (-800, 0)

  links.new(node_texcoord.outputs["UV"], node_mapping.inputs["Vector"])
  links.new(node_principled.outputs["BSDF"], node_output.inputs["Surface"])

  # Setup albedo
  if mat_config.get("albedo"):
    tex_albedo = load_image_node(nodes, mat_config["albedo"], 'sRGB')
    if tex_albedo:
      tex_albedo.location = (-300, 200)
      links.new(node_mapping.outputs["Vector"], tex_albedo.inputs["Vector"])
      links.new(tex_albedo.outputs["Color"], node_principled.inputs["Base Color"])

  # Setup roughness
  if mat_config.get("roughness"):
    tex_roughness = load_image_node(nodes, mat_config["roughness"], 'Non-Color')
    if tex_roughness:
      tex_roughness.location = (-300, -50)
      links.new(node_mapping.outputs["Vector"], tex_roughness.inputs["Vector"])
      links.new(tex_roughness.outputs["Color"], node_principled.inputs["Roughness"])

  # Setup normal map
  if mat_config.get("normal"):
    tex_normal = load_image_node(nodes, mat_config["normal"], 'Non-Color')
    if tex_normal:
      tex_normal.location = (-300, -300)
      node_normal_map = nodes.new("ShaderNodeNormalMap")
      node_normal_map.location = (-50, -250)
      
      links.new(node_mapping.outputs["Vector"], tex_normal.inputs["Vector"])
      links.new(tex_normal.outputs["Color"], node_normal_map.inputs["Color"])
      links.new(node_normal_map.outputs["Normal"], node_principled.inputs["Normal"])

def main():          
  # Clear the scene by deleting all objects
  bpy.ops.object.select_all(action='SELECT')
  bpy.ops.object.delete()
  
  # ==========================================
  # Setup rendering engine (CYCLES)
  # ==========================================
  scene = bpy.context.scene
  scene.render.engine = 'CYCLES'
  
  scene.cycles.device = 'GPU'
  prefs = bpy.context.preferences.addons["cycles"].preferences
  prefs.refresh_devices()
  has_gpu = any(d.use for d in prefs.devices if d.type != 'CPU')
  if not has_gpu:
    print("No GPU found! Falling back to CPU rendering.")
    scene.cycles.device = 'CPU'
      
  scene.cycles.samples = 32
  scene.render.resolution_x = 1280
  scene.render.resolution_y = 720 
  scene.render.filepath = OUTPUT_IMAGE

  # ==========================================
  # Import GLTF model
  # ==========================================
  try:
    bpy.ops.import_scene.gltf(filepath=GLTF_PATH)
  except Exception as e:
    print(f"Failed to import GLTF: {e}")
    exit(1)

  # ==========================================
  # Load materials from JSON configuration
  # =========================================
  if not os.path.exists(CONFIG_PATH):
    print(f"Configuration file not found in {CONFIG_PATH}")
    exit(1)
    
  with open(CONFIG_PATH, 'r') as f:
    config_data = json.load(f)

  for mat in bpy.data.materials:
    # Remove any suffixes (.001) that Blender adds in case of duplicates
    base_name = mat.name.split('.')[0] 
    if base_name in config_data["materials"]:
      print(f"Applying material: {base_name}")
      setup_material_nodes(mat, config_data["materials"][base_name])
    else:
      print(f"Material not found {base_name}. Use default texture.")

  # ==========================================
  # Camera setup
  # ==========================================
  cam_data = bpy.data.cameras.new('MainCamera')
  cam_data.lens_unit = 'FOV'
  cam_data.angle_y = math.radians(60)
  cam_obj = bpy.data.objects.new('MainCamera', cam_data)
  bpy.context.collection.objects.link(cam_obj)
  scene.camera = cam_obj

  cam_obj.location = (0.0, 0.0, 0.0)
  rot_gl = mathutils.Euler((math.radians(-180.0), math.radians(-30.0), math.radians(-180.0)), 'XYZ').to_matrix()
  
  mat_gl_to_blender = mathutils.Matrix.Rotation(math.radians(90), 3, 'X')
  rot_blender_base = mat_gl_to_blender @ rot_gl
  mat_local_pitch = mathutils.Matrix.Rotation(math.radians(0), 3, 'X')
  rot_blender_final = rot_blender_base @ mat_local_pitch
  cam_obj.rotation_euler = rot_blender_final.to_euler()


  # ==========================================
  # Lighting setup (directional light)
  # ==========================================
  light_data = bpy.data.lights.new(name="IndoorLight", type='POINT')
  light_data.energy = 700.0
  light_data.color = (1.0, 0.95, 0.88)
  light_data.shadow_soft_size = 0.15
  light_obj = bpy.data.objects.new(name="IndoorLight", object_data=light_data)
  light_obj.location = (0.0, 0.0, 0.5) 
  bpy.context.collection.objects.link(light_obj)

  # ==========================================
  # Start rendering
  # ==========================================
  bpy.ops.render.render(write_still=True)

if __name__ == "__main__":
  main()