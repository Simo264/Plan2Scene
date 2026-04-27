 The objective is to create a program in C++ which, given as input a 2D CAD model of a room, will generate a 3D mesh of the room with related materials and
 photorealistic rendering.

We therefore start from a DWG file which represents a plan of a room in two dimensions with dimensions and morphology of the room. The room will most likely not be a
perfect rectangle but will have recesses, corners, doors, etc.

Given the floor plan of the room we will have to create the corresponding model of the room in 3D. In practice we have to generate a mesh with new vertices with
associated coordinated textures. Then we will have to apply some materials to this 3D model. For the generation of coordinated textures and the application of
materials, the use of Blender is strongly recommended (use the API in C++/Python).

Note: the 3D model with coordinated textures and related materials must then be exported in GLTF format.

Once the model is ready, we proceed to perform the photorealistic rendering of the room.
Our task is not to write a photorealistic renderer, in fact we will use the Blender rendering engine. 
As before, you will need to interface with Blender using API (in C++/Python).

 ## Phase 1 - Parsing the 2D CAD model

At the beginning we start from a 2D CAD model in DWG format. However, this format is proprietary to Autodesk, it is best to first convert it 
to an open format such as DXF.

The objective of this phase is to extract the contours of the room, any holes (such as doors or columns), segments such as an ordered sequence of 2D vertices.

 ## Phase 2 - Geometry generation

Once we have a 2D polygon we need to create a 3D mesh. Since the room is not regular, we will have to triangulate an arbitrary polygon, even with any holes.

One method can be Delaunay Triangulation (good, but NOT always ideal for polygons with constraints). Possible alternatives could be Constrained Delaunay Triangulation
or ear clipping (simpler but less robust).  
After we perform the Delaunay Triangulation we get a list of floor triangles.

Once we have the floor, to create the ceiling we simply make an extrusion, where the outline of the room is extruded vertically by a height H and we 
obtain a prism solid

In this way we obtained a room with floor, ceiling and walls, practically a 3D mesh that we can export as a GLTF model.

 ## Phase 3 - UV mapping and materials

Here we start from a 3D model and our goal is to define the coordinated textures on the vertices and apply the materials.

To do this we use Blender either from GUI or via API.

## Step 4 - Headless rendering

The rendering phase takes place using the Blender engine and we can use the GUI or via API.

