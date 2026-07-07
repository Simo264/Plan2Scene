import bpy, math, os, mathutils, json

GLTF_PATH = "/home/simone/Desktop/Plan2Scene/tmp/Simple_House_Plan.gltf"
HDRI_PATH = "/home/simone/Desktop/Plan2Scene/HDRIs/partly_cloudy_4k.hdr"
CONFIG_PATH = "/home/simone/Desktop/Plan2Scene/blender_materials.json"
OUTPUT_IMAGE = "/home/simone/Desktop/Plan2Scene/tmp/output.png"

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

  # ==========================================
  # Nodi Base
  # ==========================================
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

  # Dizionario per tener traccia degli output da collegare alla fine
  mat_outputs = {}

  # ==========================================
  # 1. Albedo (DIFF)
  # ==========================================
  if mat_config.get("albedo"):
    tex_albedo = load_image_node(nodes, mat_config["albedo"], 'sRGB')
    if tex_albedo:
      tex_albedo.location = (-600, 300)
      links.new(node_mapping.outputs["Vector"], tex_albedo.inputs["Vector"])
      mat_outputs['albedo'] = tex_albedo.outputs["Color"]

  # ==========================================
  # 2. ARM (AO, Roughness, Metallic) Packed
  # ==========================================
  if mat_config.get("arm"):
    tex_arm = load_image_node(nodes, mat_config["arm"], 'Non-Color')
    if tex_arm:
      tex_arm.location = (-600, 0)
      links.new(node_mapping.outputs["Vector"], tex_arm.inputs["Vector"])
      
      # Separa i canali (Rosso=AO, Verde=Roughness, Blu=Metallic)
      node_sep = nodes.new("ShaderNodeSeparateColor")
      node_sep.location = (-350, -50)
      links.new(tex_arm.outputs["Color"], node_sep.inputs["Color"])
      
      mat_outputs['ao'] = node_sep.outputs["Red"]
      links.new(node_sep.outputs["Green"], node_principled.inputs["Roughness"])
      links.new(node_sep.outputs["Blue"], node_principled.inputs["Metallic"])

  # ==========================================
  # 3. AO Individuale (Se presente, sovrascrive quello dell'ARM)
  # ==========================================
  if mat_config.get("ao"):
    tex_ao = load_image_node(nodes, mat_config["ao"], 'Non-Color')
    if tex_ao:
      tex_ao.location = (-600, 150)
      links.new(node_mapping.outputs["Vector"], tex_ao.inputs["Vector"])
      mat_outputs['ao'] = tex_ao.outputs["Color"]

  # ==========================================
  # 4. Roughness Individuale (Sovrascrive l'ARM)
  # ==========================================
  if mat_config.get("rough"):
    tex_roughness = load_image_node(nodes, mat_config["rough"], 'Non-Color')
    if tex_roughness:
      tex_roughness.location = (-600, -150)
      links.new(node_mapping.outputs["Vector"], tex_roughness.inputs["Vector"])
      links.new(tex_roughness.outputs["Color"], node_principled.inputs["Roughness"])

  # ==========================================
  # 5. Unione Albedo + AO (Moltiplicazione)
  # ==========================================
  if 'albedo' in mat_outputs and 'ao' in mat_outputs:
    node_mix = nodes.new("ShaderNodeMixRGB")
    node_mix.blend_type = 'MULTIPLY'
    node_mix.inputs["Fac"].default_value = 1.0  # Intensità dell'AO
    node_mix.location = (-250, 200)
    
    links.new(mat_outputs['albedo'], node_mix.inputs["Color1"])
    links.new(mat_outputs['ao'], node_mix.inputs["Color2"])
    links.new(node_mix.outputs["Color"], node_principled.inputs["Base Color"])
  elif 'albedo' in mat_outputs:
    links.new(mat_outputs['albedo'], node_principled.inputs["Base Color"])

  # ==========================================
  # 6. Normal Map (NOR_GL o NORM_DX)
  # ==========================================
  if mat_config.get("normal"):
    tex_normal = load_image_node(nodes, mat_config["normal"], 'Non-Color')
    if tex_normal:
      tex_normal.location = (-600, -400)
      node_normal_map = nodes.new("ShaderNodeNormalMap")
      node_normal_map.location = (-250, -400)
      
      links.new(node_mapping.outputs["Vector"], tex_normal.inputs["Vector"])
      links.new(tex_normal.outputs["Color"], node_normal_map.inputs["Color"])
      links.new(node_normal_map.outputs["Normal"], node_principled.inputs["Normal"])

  # ==========================================
  # 7. Displacement (DISP)
  # ==========================================
  if mat_config.get("disp"):
    tex_disp = load_image_node(nodes, mat_config["disp"], 'Non-Color')
    if tex_disp:
      tex_disp.location = (-600, -650)
      node_disp = nodes.new("ShaderNodeDisplacement")
      node_disp.location = (-250, -650)
      
      links.new(node_mapping.outputs["Vector"], tex_disp.inputs["Vector"])
      links.new(tex_disp.outputs["Color"], node_disp.inputs["Height"])
      links.new(node_disp.outputs["Displacement"], node_output.inputs["Displacement"])
      
      # Indica a Cycles come gestire il displacement
      mat.cycles.displacement_method = 'BUMP' # Opzioni: 'BUMP', 'DISPLACEMENT', 'BOTH'

def setup_world_hdri(hdri_path, strength=1.0, rotation_z=0.0):
  abs_path = os.path.abspath(hdri_path)
  if not os.path.exists(abs_path):
    print(f"Warning: HDRI not found {abs_path}")
    return

  # Crea (o riusa) il World
  world = bpy.data.worlds.get("World")
  if world is None:
    world = bpy.data.worlds.new("World")
  bpy.context.scene.world = world

  world.use_nodes = True
  nodes = world.node_tree.nodes
  links = world.node_tree.links
  nodes.clear()

  # Nodi base
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

  # Carica l'immagine HDR
  img_name = os.path.basename(abs_path)
  if img_name in bpy.data.images:
      node_env_tex.image = bpy.data.images[img_name]
  else:
      node_env_tex.image = bpy.data.images.load(abs_path)

  # Le HDRI vanno caricate come 'Non-Color' (di default Blender usa già lo spazio corretto per .hdr)
  node_env_tex.image.colorspace_settings.name = 'Non-Color'

  # Collegamenti
  links.new(node_texcoord.outputs["Generated"], node_mapping.inputs["Vector"])
  links.new(node_mapping.outputs["Vector"], node_env_tex.inputs["Vector"])
  links.new(node_env_tex.outputs["Color"], node_background.inputs["Color"])
  links.new(node_background.outputs["Background"], node_output.inputs["Surface"])

  # Rotazione opzionale (utile per orientare il sole/skybox)
  node_mapping.inputs["Rotation"].default_value[2] = math.radians(rotation_z)

  # Intensità della luce ambientale
  node_background.inputs["Strength"].default_value = strength

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

  setup_world_hdri(HDRI_PATH, strength=1.0, rotation_z=0.0)

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