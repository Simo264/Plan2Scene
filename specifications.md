The main focus of the project is on computing the geometry of CAD models. The main goal is to create a program that, given as input a 2D CAD model in DXF format, creates a 3D mesh with normals. Only then, with Blender can we calculate the texture coordinates, UV mapping and apply the materials.
The CAD model can represent either a plan of a single room or a plan of an apartment with walls, windows, doors. Interiors will be ignored.
To represent doors and windows: the door must start from the floor but must not touch the ceiling, there must be some space; the window must be a hole in the middle, so it must not touch the floor and it must not touch the ceiling.

Problems: the vast majority of CAD models we will not have a single closed polyline entity to represent the wall. In fact, most likely in the model there will be windows and doors that tend to break the geometry producing disconnected SEGMENTS, POLYLINES and ARCS. 

There can be two alternative routes. The first is to represent the model as a planar straigth line graph (PSLG) and then use a Halfedge structure to extract the faces. This solution could be fine if we have a model in which the walls are represented as single lines and not as rectangles because otherwise the extracted faces are those of the walls and not of the room. The other approach could be Polygon Union: compute union of all wall polygons (with door and windows) using `Clipper2`.

We have a first step called **Vertex Snapping**: two neighboring vertices that are less than $\epsilon$ apart can be considered as a single vertex. The **Spatial Hash** data structure is intended for efficient fixed-radius proximity queries. It solves the following problem: "given a point $p$, find all points $p'$ such that $\text{distance}(p, p') < r$.

We may have to resolve inconsistency issues, such as the *T-junctions*, which occurs when the end of a segment ends in the middle of another segment. You need to detect them and split the segment into two separate segments. No vertices should be inside any segments.
Also pay attention to *dangling segments* which are those segments of internal wall that touch the border on one side but not on the other. 

Next, we need to represent the set of disconnected primitives as a **planar embedded graph**. A *Planar Graph* is a type of graph that CAN be drawn without any of its edges crossing each other. A *planar embedded graph* is a planar graph that has ACTUALLY been mapped onto a plane. Once a graph is embedded, we can formally talk about its faces. As result we obtain a *planar straight-line graph* (PSLG) that is an embedding of a planar graph in the plane such that its edges are mapped into straight-line segments.
There exist three well-known data structures for representing PSLGs, one of these is the **Halfedge**. The halfedge data structure stores both orientations of an edge and links them properly, simplifying operations and the storage scheme.
Once the Half-Edge structure is built, our output is a collection of isolated, explicit 2D polygons (faces). 

After that, we need to perform Polygon Triangulation using Constrained Delaunay Triangulation algorithm, the Polygon Offsetting algorithm, that is usefull to create the thickness effect on walls, and proceed with the extrusion of the walls. Finally, we are ready to export the 3D mesh as GLTF.

Once we have the GLTF model, we import it with Blender and calculate the the coordinated textures, UV mapping and apply the materials, and finally perform the photorealistic rendering.

## Structure of the project

The project will be divided as follows:
1. Computational geometry (C++):
  - Parsing with `libdxfrw`
  - Segment subdivision + Half-Edge with `CGAL`
  - Polygon triangulation with `poly2tri`
  - Polygon offsetting with `Clipper2`
  - Visualize the geometry with `OpenGL`
  - Exporting with `TinyGLTF`
2. Rendering with Blender (`bpy` and `BlenderProc2`)
  - Import of the GLTF model
  - Calculate texture coordinate, UV mapping and materials 
  - Headless rendering with Cycles or EEVEE