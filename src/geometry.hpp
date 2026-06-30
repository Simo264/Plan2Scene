#pragma once

#include "spatial_hashing.hpp"
#include "types.hpp"

#include <poly2tri/common/shapes.h>
#include <poly2tri/sweep/cdt.h>
#include <vector>

// Calculate the signed area of the contour using the shoelace formula.
f64 calculate_signed_area(const std::vector<glm::dvec2>& contour);

// Calculate the bounding box for 2D points
BoundingBox2D calculate_bbox_2D(const std::vector<glm::dvec2>& polyline);
BoundingBox2D calculate_bbox_2D(const Face& face);
BoundingBox2D calculate_bbox_2D(const std::vector<Segment>& segments);
BoundingBox2D calculate_bbox_2D(const std::vector<glm::dvec2>& points, const std::vector<u32>& cluster_indices);

// Returns the two long sides of the bounding box. 
// If the bounding box is wider than it is tall, it returns the top and bottom sides. 
// Otherwise, returns the left and right sides.
std::array<Segment, 2> get_long_sides_bbox2d(const BoundingBox2D& bbox);

// Calculates the bounding box for 3D points
BoundingBox3D calculate_bbox_3D(const std::vector<Vertex_PNT>& vertices);

// Calculate the unit scale based on the geometry
f64 detect_unit_scale(f64 area_bbox);

// Normalizes all segment coordinates according to the drawing's measurement unit.
// Scales the start and end spatial coordinates of every segment in the collection by a conversion factor. 
void normalize_segments(f64 unit, std::vector<Segment>& segments);

// Retrieves the topological neighbors connected to a specific vertex.
// Searches through the edge list to find the two adjacent vertices connected to the 
// target vertex. In a manifold wall layout, each inner corner or wall endpoint vertex 
// is expected to connect exactly to two edges (forming the inner and outer faces of the wall).
std::array<VertexId, 2> find_neighboors(VertexId vertex, 
                                        const std::vector<Edge>& edges);

// Identifies the corresponding wall-thickness vertex on the opposite side of a wall.
// Evaluates the two topological neighbors of a given vertex to determine which one points 
// across the wall thickness rather than continuing along the wall length. It computes the 
// absolute dot product between the door/wall longitudinal direction and the normalized direction 
// vectors of both neighbors. The neighbor with the lowest dot product (closest to being 
// perpendicular, i.e., 0) is selected as the correct opposite vertex.
VertexId get_adjacent_vertex(const glm::dvec2& wall_dir,
                             VertexId vertex_id,
                             const std::array<VertexId, 2>& vertex_neighbors, 
                             const std::vector<glm::dvec2>& vertices);

// Samples a set of 2D segments into discrete points.
// Iterates through each segment and generates a specified number of evenly spaced 
// points along its length using linear interpolation (LERP).
std::vector<glm::dvec2> sample_segments(const std::vector<Segment>& segments, 
                                        i32 num_samples);

// Identifies spatial groups within a point cloud using the DBSCAN clustering algorithm.
// Initializes and executes a DBSCAN instance configured with a minimum of 2 points per cluster.
// It groups the sampled window points based on the specified epsilon distance threshold,
// allowing individual window objects to be extracted from a disjointed set of input geometries.
std::vector<std::vector<u32>> calculate_clusters(std::vector<glm::dvec2>& sample_points,
                                                 f64 eps);

// Finds the four vertices that define the wall strip around a gap.
// Given the two endpoints of a door/window gap, this function snaps them to
// the nearest wall vertices (B and D). It then determines the adjacent
// vertices C and E along the wall direction, completing the wall strip.
WallVertices get_wall_vertices(const glm::dvec2& gap_start,
                               const glm::dvec2& gap_end,
                               const SpatialHash& hash,
                               const std::vector<Edge>& edges,
                               const std::vector<glm::dvec2>& vertices);

// Checks whether two vectors are parallel (or anti-parallel) within a tolerance.
// Two vectors are considered parallel if the absolute value of the cosine of
// the angle between them is close to 1. The tolerance is applied to the
// cosine value (e.g., 1e-4 means angle < ~ 0.1°).
bool are_parallel(const glm::dvec2& v1, 
                  const glm::dvec2& v2, 
                  f64 tol = 1e-4);

// Projects a point onto a line segment defined by two endpoints.
// The projection is computed using the parameter t along the segment.
// The result indicates whether the projection falls strictly inside the
// segment (with a small tolerance to avoid boundary issues).
ProjResult project_point_on_segment(const glm::dvec2& p,
                                    const glm::dvec2& a,
                                    const glm::dvec2& b,
                                    f64 tol = 1e-4);

// Splits an existing edge into two by inserting a new vertex.
// The original edge between v1 and v2 is removed and replaced by two edges:
// (v1, new_id) and (new_id, v2), both with the specified layer type.
// The new vertex coordinates are added to the vertices list.
VertexId split_edge(std::vector<glm::dvec2>& vertices,
                    std::vector<Edge>& edges,
                    VertexId v1,
                    VertexId v2,
                    const glm::dvec2& new_point,
                    LayerType layer = LayerType::WALL);

// Closes the wall around a door or window gap.
// 
// Given the two endpoints of a gap (door/window segment), this function:
// 1. Finds the four vertices (B,C,D,E) of the wall strip.
// 2. Checks if the opposite side (C-E) is parallel to the gap (B-D).
// 3. If not parallel, it projects E onto B-C or C onto D-E to create a new
//    vertex that makes the opposite side parallel, splitting the corresponding wall edge.
// 4. Finally, it adds two edges: one for the gap (B-D) and one for the
//    opposite side (the new parallel segment), both with the given layer type.
void close_wall_gap(glm::dvec2 gap_start,
                    glm::dvec2 gap_end,
                    LayerType type,
                    SpatialHash& hash,
                    std::vector<Edge>& edges);

// Reconstructs topological elements for doors and bridges the gaps between walls.
// Iterates through a collection of raw door segments, delegates the geometry snapping and graph
// integration to close_wall_gap, and updates the original door segment boundaries with the 
// actual snapped vertex positions from the graph.
void doors_reconstruction(std::vector<Segment>& doors,
                          SpatialHash& hash,
                          std::vector<Edge>& edges);

// Reconstructs window features by identifying bounding boxes from clustered sampled points.
// Processes the clusters generated by spatial clustering (e.g., DBSCAN) on the window sample points. 
// For each cluster, it computes a 2D bounding box, extracts its longest longitudinal side, and 
// invokes close_wall_gap to structurally seal the window opening in the graph.
void windows_reconstruction(std::vector<glm::dvec2>& sample_points,
                            std::vector<std::vector<u32>> clusters,
                            SpatialHash& hash,
                            std::vector<Edge>& edges);




void build_triangulated_face(std::vector<Vertex_PNT>& out_vertices,
                             std::vector<u32>& out_indices,
                             const std::vector<p2t::Triangle*> triangles,
                             f32 height,
                             bool facing_up, 
                             const BoundingBox2D& face_bbox);

void triangulate_face(std::vector<Vertex_PNT>& out_vertices,
                      std::vector<u32>& out_indices,
                      f32 height,
                      bool facing_up, 
                      const Face& face);


// Extrudes a 2D face contour into a 3D quad-based wall segment between two heights.
// Creates four vertices for each edge of the contour, calculates the outward-facing normal based on the edge direction,
// and pushes two triangles per edge to the index buffer.
void extrude_face(std::vector<Vertex_PNT>& vertices, 
                  std::vector<u32>& out_indices,
                  f32 base_height,
                  f32 top_height,
                  const Face& face);

void center_mesh(std::vector<Vertex_PNT>& vertices);