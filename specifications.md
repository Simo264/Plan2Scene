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
Generally speaking, it only takes one segment per door for us to rebuild doors. From that segment we find the two junctions of the walls $A \to B$. To create a closed face we need to find the other two junctions and create the other parallel segment. We will have to look for the other two junctions $C \to D$.
This is quite simple as we can search between the neighbors of node $A$ and the neighbors of node $B$. For windows, the reasoning is very similar in that we would only need to have a single segment per window to reconstruct each face of the windows.
Unlike doors where for the most part they are represented by equal BLOCKS, ARC and at most LWPolyline if it is a rectangle like an entrance door, windows can have different representations, they can be represented by 2 lines, 3 lines, 4 lines, rectangles, complex blocks, etc. Trying to write a parser based on segment counting is too complicated. The ideal approach would be based on spatial clustering and bounding box extraction.
Since we only have one set of window segments, our goal is to group them into clusters. After that, each cluster will be a window, and from each we can extract individual segments and reconstruct the faces.

Next, we create the PSLG (Half Edge) graph: first we insert the vertices of the wall segments and then using the placeholder information of the doors/windows we add new edges to the graph. This way we will expect to have a closed room with several faces and ready to triangulate.
It would also be important to indicate what each face represents, whether it represents the face of a wall, whether it represents a door, a window, or the interior area of the room.

## Structure of the project

The project will be divided as follows:
1. Computational geometry (C++):
  - Parsing with `libdxfrw`
  - Segment subdivision + Half-Edge with `CGAL`
  - Polygon triangulation with `poly2tri`
  - Polygon offsetting with `Clipper2`
2. Rendering with Blender
  - Import of the GLTF model
  - Calculate texture coordinate, UV mapping and materials 
  - Headless rendering with Cycles or EEVEE