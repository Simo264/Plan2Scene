## Introduction 
The goal of the project is as follows: to create a C++ program that takes as input a 2D CAD model, in DXF format, of a plan of a room/apartment with walls, windows, doors, and to create as a final result a 3D model of the room/apartment.

The model will be exported in GLTF format with attributes: position (xyz) normal (xyz) texture coordinates (uv). The GLTF model shall not contain materials.

Once the model is exported, we proceed with the photorealistic rendering phase in Blender.
Here, normals and textures coordinated will already be calculated. Blender is used for rendering only. The idea is to use Blender Python API (BPY) to define the scene, materials and rendering parameters and call the script from CLI like this: `blender -b -P render_scene.py`.
The renderer to use is Cycles.


## From plan to model
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
Suppose we have a door in the middle of the wall and the edges are represented by the vertices $A,B$ (right wall), $C,D$ (left wall). We only know the segment connecting the vertices $A,C$,and we denote this vector by $\vec{d}$ which would be the direction of the gate. We take the vertex $A$ and consider its neighbors and for each neighbor we calculate the direction vector $\vec{d'}$. If the dot procuct between $\vec{d}$ and $\vec{d'}$ is 0 (or very close to 0) it means that the two vectors are perpendicular and that vertex is a candidate to be the third junction of the wall. Same thing we do with the $C$ summit and its neighbors. In this way we will have obtained the two segments $\overline{AC}$ and $\overline{BD}$ and closed the face.

If, on the other hand, a door is represented by a polyline (a closed rectangle), we simply take one of the two long sides, and with that segment we repeat the procedure just described.

Windows, on the other hand, are more variable and can be represented in many ways. For this reason, the ideal approach would be based on spatial clustering and bounding box extraction. Each cluster of points will represent a window; from each cluster, I will calculate the bounding box and extract a single segment from it. To do this, I simply consider one of the two long sides of the box, using the Spatial Hashing structure I find the two vertices of the walls, and to derive the second segment I follow the procedure above.

##### Planar straight line graph and faces extraction
The PSLG graph we need to extract the faces, which we will need during triangulation and extrusion.
Furthermore, it will also be necessary to classify each face: is it a face of a wall? is this a door face? or is it a window face? We need to know why the extrusion will be different based on the type: a wall will be extruded up to the ceiling, a door will be extruded from 80% to 100% of the height, a window will be extruded from the floor up to 20% of the height and even from 80 to 100% to have a hole in the wall.

##### Triangulation and extrusion
To obtain the vertices and indices needed to build the mesh we need to perform triangulations on the faces. We use the Delaunay triangulation method.
Furthermore, the vectors of the normals and the UV coordinates will be calculated here

##### Exporting
Finally we get a 3D mesh with: position (xyz), normal (xyz), coordinated textures (uv).
We are ready to export in GLTF format.

### Used libraries
  - Parsing with `libdxfrw`
  - DBSCAN algorithm with `SimpleDBSCAN`
  - Planar Straight-Line Graph with `CGAL`
  - Polygon triangulation with `poly2tri`


## Photorealistic rendering
