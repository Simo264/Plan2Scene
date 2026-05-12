The objective is to create a program in C++ which, given as input a 2D CAD model of a room, will generate a 3D mesh of the room with related materials and photorealistic rendering.

We therefore start from a DXF file which represents a plan of a room in two dimensions with dimensions and morphology of the room. 
Here what we will have to do is extract the contours of the room, any holes (such as doors or columns) and segments as an ordered 
sequence of 2D vertices. 

Given the floor plan of the room we will have to create the corresponding model of the room in 3D. 
Here we have as input a 2-dimensional polygon, which is the result of the previous step, and from this we need to generate a 3D model.
One method can be Constrained Delaunay Triangulation. After that we get a list of floor triangles.
Once we have the floor, to create the ceiling we simply make an extrusion, where the outline of the room is extruded vertically by a 
height H and we obtain a prism solid. 
Furthermore, the walls must have a thickness (for example 50cm). To obtain a thickness in the walls use the polygon offsetting algorithm.

*Tip: after performed the polygon offsetting, we have generated inner and outer vertical wall faces but with no top surface and no bottom surface. The mesh is still an open shell. For a room mesh, bottom cap is unnecessary because the floor completely covers it and nobody will ever see it. We only need the top wall cap. We already have the inner polygon and outer polygon, the recommended way is to triangulate the area between them reusing `poly2tri`.*

Finally, we export the model in GLTF format with the following vertex information: position (xyz) and normals (xyz).

*Tip: when you generate the geometry you will also have to analytically calculate the normals on the vertices: the normals of the floor must point upwards and the ceiling downwards, while the walls must point towards the inside of the room.*

Once we have the GLTF model, we import it with Blender and calculate the the coordinated textures, UV mapping and apply the materials.
Once the model is ready, we proceed to perform the photorealistic rendering of the room.
Our task is not to go and write a photorealistic renderer, for this we will use the Blender renderer.

*Note: the use of Blender is strongly recommended (or use the Python API).*

## Structure of the project

The project will be divided as follows:
1. parsing + geometry (C++):
  - Reading and parse DXF file with `libdxfrw`
  - Triangulation of the floor with `poly2tri`
  - Polygon offsetting with `Clipper2`
  - Wall extrusion
  - Visualize the result geometry with `OpenGL`, `GLFW` and `ImGui`
  - Exporting the mesh in GLTF format with `TinyGLTF`
2. Texture coordinate, UV mapping, materials, rendering (Python with `bpy`)
