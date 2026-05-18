The main gola is to create a program that, given as input a 2D CAD model (in DXF format), creates a 3D mesh with related materials and photorealistic renderings.
The CAD model can represent either a plan of a single room or a plan of an apartment where there will be walls, windows, doors, furnishings. 
Initially the main thing is to build a credible model of the geometry, after which we move on to the reconstruction of the windows and doors. Interiors will be ignored.

To represent doors and windows, at the moment it may be fine to represent them simply with slots or with simple quads. The door must start from the floor but must not touch the ceiling, there must be some space. While a window must be a hole in the middle, so it must not touch the floor and it must not touch the ceiling.

We will have an initial phase in which to parse the DXF file to collect the primitives of interest: SEGMENTS, POLYLINES, ARCH which represent the walls, windows and doors, as an collection of 2D vertices.

Next, we will have a phase of polygon reconstruction to have a polygon that is suitable for floor triangulation. The reason is that in the vast majority of CAD models we will not have a single closed polyline entity to represent the wall. In fact, most likely in the model there will be windows and doors that tend to "break" the geometry producing disconnected SEGMENTS and POLYLINES.  

Once you have a clean polygon, then we will proceed with the triangulation of the floor using the Constrained Delaunay Triangulation algorithm (the ceiling is nothing more than the floor translated upwards).

Next, run the Polygon Offsetting algorithm which will generate a new external contour. It is usefull to create the thickness effect on walls.
Then we will proceed with the extrusion of the walls, which simply translates the contour of the walls by a height H. Afterwards it will be necessary to tie (only the upper part) the external contour with the internal one by re-performing the triangulation between the two contours.

Finally, we obtain a 3D mesh and we are ready to export the model in GLTF format with the following vertex information: position (xyz) and normals (xyz).

Once we have the GLTF model, we import it with Blender and calculate the the coordinated textures, UV mapping and apply the materials.
Once the model is ready, we proceed to perform the photorealistic rendering of the room.
Our task is not to go and write a photorealistic renderer, for this we will use the Blender renderer.

> Tip: it is recommended to use the Blender API's

> Tip: regarding the rendering phase, consider using BlenderProc2.

## Structure of the project

Blender does not expose native APIs for C++. The only official scripting modes are for Python.

The project will be divided as follows:
1. parsing + geometry (C++):
  - Parse DXF file with `libdxfrw`
  - Polygon reconstruction with `CGAL`
  - Triangulation of the floor with `poly2tri`
  - Polygon offsetting with `Clipper2`
  - Wall extrusion
  - Visualize the result geometry with `OpenGL`, `GLFW` and `ImGui`
  - Exporting the mesh in GLTF format with `TinyGLTF`
2. Texture coordinate, UV mapping, materials, rendering (using with `bpy` API in Python)
  - Import of the GLTF model
  - Calculate texture coordinate and UV mapping
  - Application of materials 
  - Headless rendering with Cycles or EEVEE