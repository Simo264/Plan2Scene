The goal of the project is as follows: to create a C++ program that takes as input a 2D CAD model, in DXF format, of a plan of a room/apartment with walls, windows, doors, and to create as a final result a finite 3D model of the room/apartment.
For the moment, furnishings such as TV, sofa, bed, etc. are ignored. The walls will be extruded, the doors and windows will be represented as simple cracks between the walls. This is the most complex part of the project.
Once this part is complete, there will be a second part of Blender where you can import the newly created model, define UV mapping, apply the materials to conclude with the photorealistic rendering of the model.

The most difficult thing about this project is the lack of a single standard among CAD models, which makes it very difficult to create an algorithm that can generalize all models. Mainly on how they represent walls, doors and windows.

Let's assume that the models we take as examples represent walls as already closed loops and not as single lines. The walls will also be separated from each other due to the presence of doors and windows.
If we take the segments of the doors and windows then the walls will be connected to each other and will create a closed geometry.
As for doors, very often they are either arches or segments that connect the two joints of the walls.
Per le finestre invece la cosa si complica perché possono essere rappresentate in tanti modi diversi. We assume that they are segments that connect the joints of the walls.

Initially we will need to parse the file to collect all the primitives we are interested in: walls, doors, windows. Door and window information serves us more as a placeholder than as actual geometry

After that we have a **vertex snapping** phase, only on wall primitives, where we collapse the vertices into a single vertex if they are within the radius $\epsilon$, using the Spatial Hashing structure.

After snapping the vertices on the walls, we need to proceed with reconstructing the faces of the doors and windows that will be extracted in the next phase, and are used to close the outline of the room or house.
Generally speaking, to reconstruct a face of a door or window we only need a segment that connects the two edges of the walls. From that single segment we are able to derive the other two edges and insert the other segment so as to form a closed face. This is quite simple. Suppose we have a door in the middle of the wall and the edges are represented by the vertices $A,B$ (right wall), $C,D$ (left wall). We only know the segment connecting the vertices $A,C$,and we denote this vector by $\vec{d}$ which would be the direction of the gate. We take the vertex $A$ and consider its neighbors and for each neighbor we calculate the direction vector $\vec{d'}$. If the dot procuct between $\vec{d}$ and $\vec{d'}$ is 0 (or very close to 0) it means that the two vectors are perpendicular and that vertex is a candidate to be the third junction of the wall. Same thing we do with the $C$ summit and its neighbors. In this way we will have obtained the two segments $\overline{AC}$ and $\overline{BD}$ and closed the face. This applies to both doors and windows.

Ports have a fairly standard representation and are usually pointers to BLOCKS, and the primitives used are usually ARC. Sometimes however they can be LINE or LWPolyline. Windows, on the other hand, are more variable and can be represented in many ways. For this reason, the ideal approach would be based on spatial clustering and bounding box extraction. Each cluster of points will represent a window; from each cluster, I will calculate the bounding box and extract a single segment from it.
To do this, I simply consider one of the two long sides of the box, using the Spatial Hashing structure I find the two vertices of the walls, and to derive the second segment I follow the procedure above.

Next, we create the PSLG (Half Edge) graph: first we insert the vertices of the wall segments and then using the placeholder information of the doors/windows we add new edges to the graph. This way we will expect to have a closed room with several faces and ready to triangulate.
It would also be important to indicate what each face represents, whether it represents the face of a wall, whether it represents a door, a window, or the interior area of the room.

## Structure of the project

The project will be divided as follows:
1. Computational geometry (C++):
  - Parsing with `libdxfrw`
  - DBSCAN algorithm with `SimpleDBSCAN`
  - Planar Straight-Line Graph with `CGAL`
  - Polygon triangulation with `poly2tri`
  - Polygon offsetting + boolean op with `Clipper2`
2. Rendering with Blender
  - Import of the GLTF model
  - Calculate texture coordinate, UV mapping and materials 
  - Headless rendering with Cycles or EEVEE