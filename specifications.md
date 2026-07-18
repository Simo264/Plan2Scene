## Introduction

The goal of the project is as follows: to create a C++ program that takes as input a 2D CAD model, in DXF format, of a plan of a room/apartment with walls, windows, doors, and to create as a final result a 3D model of the room/apartment.

The model will be exported in GLTF format with attributes: position (xyz) normal (xyz) texture coordinates (uv). The GLTF model shall not contain materials.

Once the model is exported, we proceed with the photorealistic rendering phase in Blender. Blender is used for rendering only. The idea is to use Blender Python API (BPY) to define the scene, materials and rendering parameters and call the script from CLI like this: `blender -b -P render_scene.py`.

## From plan to mesh

The most difficult thing about this project is the lack of a single standard among CAD models, which makes it very difficult to create an algorithm that can generalize all models.

That's why we need to make assumptions: walls must already be closed faces with their own thickness. Important: Every face of the wall must be closed, I can't have a crack in a wall.
The walls will be extruded, the doors and windows will be represented as holes in walls.

##### Preprocessing

Even before running the program you will need to open the model with software like LibreCAD to make it suitable: the layer names must be correct, if necessary close cracks in the faces of the walls, add any new vertices and segments, or modify the windows.

##### Parsing

At this stage we need to collect all the information regarding walls, doors and windows.
Walls are very often represented as segments or as polylines. 
Doors very often point to the same BLOCK, which can be the same arc. But it can happen that a port is also a set of segments or polylines (in the case of input ports). 
Windows, like doors, can point to the same BLOCK. Windows can be represented in many different ways, but typically they are represented as segments.

*Hint: Door and window information serves us more as a placeholder than as actual geometry.*

##### Vertex snapping

Only the primitives of the walls are processed. Neighboring vertices that are less than $\epsilon$ are collapsed into a single vertex. The SpatialHash data structure is used.
This step is particularly useful both to reduce the number of vertices to be processed after, and to correct any problems with overlapping vertices that could lead to problems during graph creation and consequently face extraction.

##### Gaps reconstruction

Here we need to close the holes to reconstruct the faces relating to doors and windows.

As for the doors, we only know the segments that connect the two edges of the walls. To create a face we need to calculate the second parallel segment.
From that single segment, we can derive the other two vertices. It's actually quite simple.

If a door is represented by a polyline (a closed rectangle), we simply take one of the two long sides, and calculate the second parallel segment.

Windows, on the other hand, are more variable and can be represented in many ways. For this reason, the ideal approach would be based on spatial clustering and bounding box extraction. Each cluster of points will represent a window; from each cluster, I will calculate the bounding box and extract a single segment from it. To do this, I simply consider one of the two long sides of the box, using the Spatial Hashing structure I find the two vertices of the walls, and to derive the second segment I follow the procedure above.

##### Planar straight line graph and faces extraction

The PSLG graph we need to extract the faces, which we will need during triangulation and extrusion.
Furthermore, it will also be necessary to classify each face: is it a face of a wall? is this a door face? or is it a window face? We need to know why the extrusion will be different based on the type: a wall will be extruded up to the ceiling, a door will be extruded from 80% to 100% of the height, a window will be extruded from the floor up to 20% of the height and even from 80 to 100% to have a hole in the wall.

##### Triangulation and extrusion

To obtain the vertices and indices needed to build the mesh we need to perform triangulations on the faces. We use the Delaunay triangulation method.
Furthermore, the normal vectors and the textures coordinates will be calculated here.

##### Exporting

Finally we get a 3D mesh with: position (xyz), normal (xyz) and textures coordinates (uv).
We are ready to export in GLTF format.

### Libraries

  - Parsing with `libdxfrw`
  - DBSCAN algorithm with `SimpleDBSCAN`
  - Planar Straight-Line Graph with `CGAL`
  - Polygon triangulation with `poly2tri`

## Definition of parameters

The C++ program reads all its parameters from a **JSON configuration file**.  
Each CAD model requires its own configuration file, as parameters may vary depending on the drawing scale, geometry complexity, and desired output quality. Example:

```json
{
  "dxf_filename": "draftperson_Floor_Plan.dxf",
  "unit_scale": 0.01,
  
  "ceil_height": 3.5,
  "door_width": 1.2,
  "door_height": 2.1,
  
  "window_sill_height": 0.25,
  "window_height": 3.3,
  "window_width": 5.0,

  "snap_eps": 1e-2,
  "cluster_num_samples": 15,
  "cluster_eps": 2,

  "floor_texture_scaling": 2.0,
  "wall_texture_scaling": 2.0
}
```

- *dxf_filename*: path to the input DXF file.
- *unit_scale*: conversion factor from DXF drawing units to meters (e.g., 0.01 if the drawing is in centimeters).
- *ceil_height*: height of the ceiling in meters.
- *door_width*, door_height: target dimensions for doors.
- *window_sill_height*: distance from floor to window sill.
- *window_height*, window_width: target dimensions for windows.
- *snap_eps*: tolerance for vertex snapping (in meters).
- *cluster_num_samples*: minimum points for DBSCAN clustering.
- *cluster_eps*: maximum distance for DBSCAN clustering.
- *floor_texture_scaling*, wall_texture_scaling: scaling factors for UV coordinates to repeat textures.

During the geometry processing phase, the calculation engine does not apply a static scaling factor, but dynamically calculates the ratio of the width detected in the DXF to the desired target width to bring each gap back to the exact required geometric measurement, while preserving the midpoint (centering) of the original opening.

After the mesh is exported in GLTF format (with accompanying .bin file), the program also generates an openings.json file containing metadata for each detected opening:
```json
{
  "openings": [
    {
      "center": [0,0,0],
      "height": 2,
      "rotation_z": 3.14,
      "thickness": 0.9,
      "type": "Door",
      "width": 1.3
    }
  ]
}
```

This file provides the position, orientation, dimensions, and type of each opening, and is later used by the Blender rendering script to place 3D door/window assets accurately.

## Photorealistic rendering

The final 3D model is exported as a GLTF file containing vertex positions, normals, and texture coordinates (UV).  
It does not include any material definition inside the file.

The photorealistic rendering pipeline is implemented using Blender and its Python API (bpy).  
The render engine used is Cycles, which provides physically‑based ray tracing for high‑quality results.

The setup process is driven by a `blender_config.json` file, which centralises all scene parameters, assets, and material paths. Below is an example of its structure:

```json
{
  "samples": 128,
  "resolution_x": 1280,
  "resolution_y": 720,
  "use_denoising" : true,
  "render_engine": "CYCLES",
  "output_blender": "out/scene.blend",

  "model_path": "out/draftperson_Floor_Plan.gltf",
  "opening_placeholders": "out/draftperson_Floor_Plan_openings.json",
  "door_asset": "assets/Wooden_Door.glb",
  "window_asset": "assets/Classical_Window.glb",
  "hdri_path": "HDRIs/grasslands_sunset_4k.hdr",
  "hdri_intensity": 1.0,

  "floor_material": {
    "albedo": "materials/patio_tiles/patio_tiles_diff_1k.jpg",
    "base_color": [0.8, 0.8, 0.8],
    "normal": "...",
    "normal_strength": 1.5,
    "roughness": "...",
    "roughness_value": 0.5,
    "metallic": "...",
    "metallic_value": 0.0,
    "displacement": "...",
    "disp_scale": 0.05,
    "ao": "...",
    "ao_mix_factor": 1.0,
    "arm": "..."
  },

  "wall_and_ceil_material": {
    "albedo": "materials/beige_wall/beige_wall_001_diff_1k.jpg",
    "base_color": [0.8, 0.8, 0.8],
    "normal": "...",
    "normal_strength": 1.5,
    "metallic": "",
    "metallic_value": 0.0,
    "roughness": "...",
    "roughness_value": 0.5,
    "displacement": "...",
    "disp_scale": 0.05,
    "ao": "...",
    "ao_mix_factor": 1.0,
    "arm": "..."
  }
}
```

The render engine defaults to CYCLES to achieve photorealism. All rendering parameters (samples, resolution, denoising) are taken directly from this file.

The dedicated Python script (render_scene.py) reads blender_config.json and performs the following operations:

1. imports the GLTF mesh located at model_path
2. reads the opening_placeholders file (i.e., the openings.json generated by the C++ program) and builds a list of OpeningInfo. For each entry, it instantiates the corresponding asset (door_asset or window_asset) with the exact position, rotation, and scale computed during the C++ processing phase.
3. assigns the specified PBR textures (albedo, normal, roughness, metallic, displacement, AO, ARM) to the correct material slots, using the GLTF material names as keys.
4. sets up the environment lighting using the provided HDRI and intensity.


This script does not produce a final image directly. Instead, it saves a complete Blender project file (.blend) at the path specified by output_blender. This gives the user the flexibility to open the scene in Blender GUI, fine‑tune camera angles, add/adjust lights, and tweak any material settings before launching the final high‑quality render manually.

The script is invoked from the command line as follows:
```bash
blender -b -P generate_blender_scene.py
```