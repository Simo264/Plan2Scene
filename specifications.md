The objective is to create a program that, given as input a 2D CAD model (in DXF format), creates a 3D mesh with related materials and photorealistic renderings.
The CAD model can represent either a plan of a single room or a plan of an apartment where there will be walls, windows, doors, notes, measurements, interiors and furnishings. 
Initially the main thing is to build a credible model of the geometry, after which we move on to the reconstruction of the windows and doors. Interior furnishings such as furniture, shelves, etc. will be ignored (for now).

To represent doors and windows, at the moment it may be fine to represent them simply with slots or with simple quads. The door must start from the floor but must not touch the ceiling, there must be some space. While a window must be a hole in the middle, so it must not touch the floor and it must not touch the ceiling.

The walls must not be two-dimensional flat planes but must have a defined thickness (e.g. 50cm). To obtain this effect you will need to implement the Polygon Offsetting algorithm.

Initially we will start from the parsing phase of the model. Here what we will have to do is extract the contours of the room (walls), holes (such as columns), archs (for the doors) and segments (for windows) as an ordered  sequence of 2D vertices. 

Initially it will start from the walls, extracting the polylines to carry out the triangulation and generate the floor. The Constrained Delaunay Triangulation algorithm will be useful to obtain triangles and vertices.
The ceiling is nothing more than the floor translated upwards and with the normals downwards.

Then we will proceed with the extrusion of the walls, simply duplicate the vertices of the contour of the walls and translate them by a height H.
To create the thickness effect, run the Polygon Offsetting algorithm which will generate a new external contour. Afterwards it will be necessary to tie (only the upper part) the external contour with the internal one by re-performing the triangulation between the two contours.

In this way we obtained a clean mesh with floor, ceiling and walls (with thickness).

> Note: during parsing it is very likely that the geometry may have holes due to the presence of doors and windows. If we extract from the primitives in which these elements are present the points that we know to be present within the geometry to be reconstructed, or even in the edges, in theory any mesh reconstruction algorithm should work (i.e High-Fidelity Lightweight Mesh Reconstruction from Point Clouds). In some cases it is also possible to do only a Delauney triangulation.

Finally, we export the model in GLTF format with the following vertex information: position (xyz) and normals (xyz).

> Tip: when you generate the geometry you will also have to analytically calculate the normals on the vertices: the normals of the floor must point upwards and the ceiling downwards, while the walls must point towards the inside of the room.

Once we have the GLTF model, we import it with Blender and calculate the the coordinated textures, UV mapping and apply the materials.
Once the model is ready, we proceed to perform the photorealistic rendering of the room.
Our task is not to go and write a photorealistic renderer, for this we will use the Blender renderer.

> Tip: it is recommended to use the Blender API's

> Tip: regarding the rendering phase, consider using BlenderProc2 (a procedural Blender pipeline for photorealistic rendering).

## Structure of the project

Blender does not expose native APIs for C++. The only official scripting modes are for Python.

The project will be divided as follows:
1. parsing + geometry (C++):
  - Reading and parse DXF file with `libdxfrw`
  - Triangulation of the floor with `poly2tri`
  - Polygon offsetting with `Clipper2`
  - Wall extrusion
  - Visualize the result geometry with `OpenGL`, `GLFW` and `ImGui`
  - Exporting the mesh in GLTF format with `TinyGLTF`
2. Texture coordinate, UV mapping, materials, rendering (Python with `bpy`)
  - Import of the GLTF model
  - Calculate texture coordinate and UV mapping
  - Application of materials 
  - Headless rendering with Cycles or EEVEE