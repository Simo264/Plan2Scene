import bpy
import math
import os
import mathutils

GLTF_PATH = "/home/simone/Desktop/Plan2Scene/Simple_House_Plan.gltf"
FLOOR_TEX = "/home/simone/Desktop/Plan2Scene/materials/interior_tiles/interior_tiles_diff_1k.jpg"
WALL_TEX = "/home/simone/Desktop/Plan2Scene/materials/concrete_layers/concrete_layers_diff_1k.jpg"
OUTPUT_IMAGE = "/home/simone/Desktop/Plan2Scene/rendering/output.png"


def setup_material_nodes(mat, image_path):
  mat.use_nodes = True
  nodes = mat.node_tree.nodes
  links = mat.node_tree.links
  nodes.clear()

  node_output     = nodes.new("ShaderNodeOutputMaterial")
  node_principled = nodes.new("ShaderNodeBsdfPrincipled")
  node_tex        = nodes.new("ShaderNodeTexImage")
  node_mapping    = nodes.new("ShaderNodeMapping")
  node_texcoord   = nodes.new("ShaderNodeTexCoord")

  node_output.location     = ( 400,  0)
  node_principled.location = (  50,  0)
  node_tex.location        = (-300,  0)
  node_mapping.location    = (-550,  0)
  node_texcoord.location   = (-800,  0)

  abs_path = os.path.abspath(image_path)
  if os.path.exists(abs_path):
    img_name = os.path.basename(abs_path)
    if img_name in bpy.data.images:
      node_tex.image = bpy.data.images[img_name]
    else:
      node_tex.image = bpy.data.images.load(abs_path)
  else:
    print(f"Warning: image not found {abs_path}")

  links.new(node_texcoord.outputs["UV"],    node_mapping.inputs["Vector"])
  links.new(node_mapping.outputs["Vector"], node_tex.inputs["Vector"])
  links.new(node_tex.outputs["Color"],      node_principled.inputs["Base Color"])
  links.new(node_principled.outputs["BSDF"], node_output.inputs["Surface"])

  node_mapping.inputs["Scale"].default_value = (1.0, 1.0, 1.0)

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
  # scene.cycles.use_denoising = True
  # scene.cycles.denoiser = 'OPENIMAGEDENOISE'
  scene.render.resolution_x = 1920
  scene.render.resolution_y = 1080 
  scene.render.filepath = OUTPUT_IMAGE

  # ==========================================
  # Import GLTF model
  # ==========================================
  bpy.ops.import_scene.gltf(filepath=GLTF_PATH)
  for mat in bpy.data.materials:
    if mat.name.startswith("FloorMaterial"):
      setup_material_nodes(mat, FLOOR_TEX)
    elif mat.name.startswith("WallMaterial"):
      setup_material_nodes(mat, WALL_TEX)


  # ==========================================
  # Camera setup
  # ==========================================
  cam_data = bpy.data.cameras.new('MainCamera')
  cam_data.lens_unit = 'FOV'
  cam_data.angle_y = math.radians(45)
  cam_obj = bpy.data.objects.new('MainCamera', cam_data)
  bpy.context.collection.objects.link(cam_obj)
  scene.camera = cam_obj

  cam_obj.location = (0.0, 0.0, 1.2)
  rot_gl = mathutils.Euler((
    math.radians(-175.0), 
    math.radians(-30.0), 
    math.radians(180.0)
  ), 'XYZ').to_matrix()
  
  mat_gl_to_blender = mathutils.Matrix.Rotation(math.radians(90), 3, 'X')
  rot_blender_base = mat_gl_to_blender @ rot_gl
  mat_local_pitch = mathutils.Matrix.Rotation(math.radians(-15), 3, 'X')
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
  light_obj.location = (0.0, 0.0, 1.5) 
  bpy.context.collection.objects.link(light_obj)

  # ==========================================
  # Start rendering
  # ==========================================
  print("Inizio il rendering con Cycles...")
  bpy.ops.render.render(write_still=True)
  print(f"Rendering completato! Salvato in {OUTPUT_IMAGE}")

if __name__ == "__main__":
  main()