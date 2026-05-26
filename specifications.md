The main focus of the project is on computing the geometry of CAD models. The main goal is to create a program that, given as input a 2D CAD model in DXF format, creates a 3D mesh. With Blender can we calculate the texture coordinates, UV mapping and apply the materials.

Since CAD models do not follow a precise standard, we can have CAD models made by different architects and which can be different from each other. I have to consider which standard is most used in most cases.
First of all, we have a pre-processing phase where we filter the layers. Layer names are not standardized. We can view the model with an online application and see the walls, windows and doors as they are named and, based on this, directly rename the layers within the DXF file since it is a text file. Layers named "WALL" concern the walls, the "DOOR" layers concern the doors, the "GLAZ" layers concern the window glass (the internal parallel lines) and "GLAZ-SILL" the two parallel lines perpendicular to the wall.

> How are walls, windows and doors represented in most cases?

In most cases, the wall is already a closed shape with thickness. It's drawn as a single closed polyline (or segments) that encloses the wall's area, with a deliberate break where the door goes. The wall's geometry (thickness, corners, overall shape) is already defined by its outer and inner boundaries.
For windows, usually there is always a gap between the walls. The most common pattern is that there are two lines parallel and perpendicular to the wall, plus one/two lines parallel to the wall (the glass). Many architects use inserted symbol blocks as INSERT entities.
Finally, as far as doors are concerned, in most cases, doors are represented by a gap in the wall, a line representing the door leaf and an arc representing the opening radius. Or, another way, the door is simply a rectangle, usually narrower, and placed near one end of the wall. In this case the information of the opening direction is lost. Here too, many architects use inserted symbol blocks as INSERT entities.

Let's consider the simplest case: a simple model of a bathroom with one continuous thick wall polygon with an opening to make room for a door. The wall is already delimited and closed, it has its own internal area, but at its ends there is a gap to make room for the door. The door is a single segment with no thickness.
Walls are LINE primitives only, the door has only one ARC primitive.

Broadly speaking, a possible idea could be the following.
Perform a layer preprocessing step, that is, open the DXF file and rename the layers. Collect the primitives of interest: walls, windows, doors. Usually these are segments. Perform vertex collapse within $\epsilon$ (vertex snapping) using the spatial hash data structure.
I could build a PSLG + Half edge graph to extract the faces, what I get would only be a single face representing the internal area of ​​the wall and not the room. I could calculate the centerlines and join the port segment and build the PSLG graph. I could take the door segment (without thickness) and apply a thickness with `Clipper2` and perform the Union operation between the wall segments with the door segment. This is the tricky part. There is no solution yet.

Dopo che abbiamo ottenuto le facce, della stanza e dei muri, possiamo procedere con la triangolazione ed estrusione dei muri. Attenzione però ai tipi di segmenti, perché muri, finestre, porte devono essere generati in modo differente! The door must start from the floor but must not touch the ceiling, there must be some space; the window must be a hole in the middle, so it must not touch the floor and it must not touch the ceiling. To represent the holes in the walls we can also consider using the Difference operation.

Finally, we are ready to export the 3D mesh as GLTF. Once we have the GLTF model, we import it with Blender and calculate the the coordinated textures, UV mapping and apply the materials, and finally perform the photorealistic rendering.

## Structure of the project

The project will be divided as follows:
1. Computational geometry (C++):
  - Parsing with `libdxfrw`
  - Segment subdivision + Half-Edge with `CGAL`
  - Boolean Union with `Clipper2`
  - Polygon triangulation with `poly2tri`
  - Polygon offsetting with `Clipper2`
  - Visualize the geometry with `OpenGL`
  - Exporting with `TinyGLTF`
2. Rendering with Blender (`bpy` and `BlenderProc2`)
  - Import of the GLTF model
  - Calculate texture coordinate, UV mapping and materials 
  - Headless rendering with Cycles or EEVEE