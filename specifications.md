The main goal is to create a program that, given as input a 2D CAD model (in DXF format), creates a 3D mesh with related materials and photorealistic renderings.
The CAD model can represent either a plan of a single room or a plan of an apartment where there will be walls, windows, doors, furnishings. 
Initially the main thing is to build a credible model of the geometry, after which we move on to the reconstruction of the windows and doors. Interiors will be ignored.

To represent doors and windows, at the moment it may be fine to represent them simply with slots or with simple quads. The door must start from the floor but must not touch the ceiling, there must be some space. While a window must be a hole in the middle, so it must not touch the floor and it must not touch the ceiling.

We will have an initial phase in which to parse the DXF file to collect the primitives of interest: SEGMENTS, POLYLINES, ARCH which represent the walls, windows and doors, as an collection of 2D vertices. 
The vast majority of CAD models we will not have a single closed polyline entity to represent the wall. In fact, most likely in the model there will be windows and doors that tend to "break" the geometry producing disconnected SEGMENTS and POLYLINES. We are facing is a computational geometry problem that is **polygon reconstruction from segments**. 

To resolve this problem, once we have the complete list of segments we must merging the vertices (**vertex snapping**): two neighboring vertices that are less than $\epsilon$ apart can be considered as a single vertex.
The **Spatial Hash** data structure is intended for efficient fixed-radius proximity queries. It solves the following problem: "given a point $p$, find all points $p'$ such that $\text{distance}(p, p') < r$.

We need to resolve all inconsistencies, such as the **T-junctions**, which occurs when the end of a segment ends in the middle of another segment. You need to detect them and split the segment into two separate segments. This process is called *segment subdivision*. After this step, no vertices should be inside any segments. `CGAL` is particularly useful for this.
Also pay attention to **dangling segments** which are those segments of internal wall (dividers) that touch the border on one side but not on the other. It is necessary to identify them and discard them.

Next, we need to represent the set of disconnected primitives as a **planar embedded graph**. Some distinctions: a *Planar Graph* is a type of graph that CAN be drawn on a flat surface (such as a piece of paper) without any of its edges crossing each other. A *planar embedded graph* is a planar graph that has ACTUALLY been mapped onto a plane such that no two edges intersect except at their common vertices. Once a graph is embedded, we can formally talk about its faces.
The goal is to build a valid planar embedded graph from which polygonal faces can emerge naturally. As result we obtain a *planar straight-line graph* (PSLG), often called *planar subdivisions*, that is an embedding of a planar graph in the plane such that its edges are mapped into straight-line segments.
There exist three well-known data structures for representing PSLGs, one of these is the **Halfedge**. The halfedge data structure stores both orientations of an edge and links them properly, simplifying operations and the storage scheme.
Once the Half-Edge structure is built, our output isn't just a list of lines anymore, but is a collection of isolated, explicit 2D polygons (faces). 

After that, we need to perform Polygon Triangulation using Constrained Delaunay Triangulation algorithm, the Polygon Offsetting algorithm, that is usefull to create the thickness effect on walls, and proceed with the extrusion of the walls by a height H (re-perform Polygon Triangulation to cap the top of those walls).
Finally, we are ready to export the 3D mesh as GLTF with the following vertex information: position (xyz) and normals (xyz).

Once we have the GLTF model, we import it with Blender and calculate the the coordinated textures, UV mapping and apply the materials.
Once the model is ready, we proceed to perform the photorealistic rendering of the room.
Our task is not to go and write a photorealistic renderer, for this we will use the Blender renderer.

> Tip: consider to use the Blender API's and BlenderProc2

## Structure of the project

Blender does not expose native APIs for C++. The only official scripting modes are for Python.

The project will be divided as follows:
1. parsing + geometry (C++):
  - Parse DXF file with `libdxfrw`
  - Segment subdivision + Half-Edge with `CGAL`
  - Polygon triangulation with `poly2tri`
  - Polygon offsetting with `Clipper2`
  - Wall extrusion
  - Visualize the result geometry with `OpenGL`, `GLFW` and `ImGui`
  - Exporting the mesh in GLTF format with `TinyGLTF`
2. Texture coordinate, UV mapping, materials, rendering (using with `bpy` API in Python)
  - Import of the GLTF model
  - Calculate texture coordinate and UV mapping
  - Application of materials 
  - Headless rendering with Cycles or EEVEE