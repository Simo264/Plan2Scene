The main focus of the project is on computing the geometry of CAD models. The main goal is to create a program that, given as input a 2D CAD model in DXF format, creates a 3D mesh. With Blender can we calculate the texture coordinates, UV mapping and apply the materials.

Since CAD models do not follow a precise standard, we can have CAD models made by different architects and which can be different from each other. I have to consider which standard is most used in most cases.
First of all, we have a pre-processing phase where we filter the layers. Layer names are not standardized. We can view the model with an online application and see the walls, windows and doors as they are named and, based on this, directly rename the layers within the DXF file since it is a text file.
Next, how are walls, windows and doors represented in most cases? In most cases, a closed polyline or as distinct parallel segments is used to represent walls in an architectural DXF file. The best thing is to convert all the primitives into simple segments and do graph building.
For windows, usually there is always a gap between the walls. The most common pattern is that there are two lines parallel and perpendicular to the wall, plus one/two lines parallel to the wall (the glass). Many architects use inserted symbol blocks as INSERT entities.
Finally, as far as doors are concerned, in most cases, doors are represented by a gap in the wall, a line representing the door leaf and an arc representing the opening radius. Or, another way, the door is simply a rectangle, usually narrower, and placed near one end of the wall. In this case the information of the opening direction is lost. Here too, many architects use inserted symbol blocks as INSERT entities.

We start from the layer parsing and classification phase, where we collect primitives of interest: walls, windows and doors. We need to perform vertex snapping on the raw segments of the DXF.
From the walls we extract the middle lines and the thickness.
Are (rectangular) walls represented by Polylines or as simple segments? The centerlines must be calculated anyway because we need them later. I might consider using straight skeleton algorithm or medial axis or pairing algorithm. 
It would be good to carry out Vertex Snapping and T-junction repair on the centerlines which is important to have a correct PSLG.
We identify any gaps caused by doors/windows. We need to verify that two segments are collinear. For this we can exploit vector algebra. Once we find the gap, we need to distinguish between window or door and generate a segment to close the gap. 
It will be important to maintain information on the type of opening because we will need it during extrusion. 
We use CGAL Arrangement for building PSLG + Half-Edge. We get a list of room faces. Now that you have the rooms, rebuild the walls as 2D rectangles. After we have generated the wall rectangles we can use the Union operation to merge adjacent rectangles and eliminate any overlaps and create a single, clean outline. We can triangulate the faces of the floor, we raise the walls but pay attention to the openings because we have to represent the doors and windows differently. To represent the holes in the walls we can also consider using the Difference operation.

> Note: during the extrusione, the door must start from the floor but must not touch the ceiling, there must be some space; the window must be a hole in the middle, so it must not touch the floor and it must not touch the ceiling.

Some considerations:
1. The face extracted from the arrangement (delimited by the centerlines) is larger than the actual room area. You have to offset inward by thickness/2 on the room-polygon to get the real internal perimeter. You need to offset outward by thickness/2 to get the outer perimeter of the walls
2. If you leave gaps open in the arrangement, the faces don't close properly and the Half-Edge doesn't work. But if you close the gaps with dummy segments you lose the door/window information. The solution is to close the gaps but tag the closing segments: Segment{ p0, p1, type }. Gap closing segments are inserted into the arrangement with type DOOR_OPENING or WINDOW_OPENING. Geometrically they close the PSLG, but semantically you know there's an opening there. In CGAL you can attach this information as given to the halfedge.

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