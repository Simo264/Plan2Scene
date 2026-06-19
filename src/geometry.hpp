#pragma once

#include "spatial_hashing.hpp"
#include "types.hpp"

#include <poly2tri/common/shapes.h>
#include <poly2tri/sweep/cdt.h>
#include <vector>

struct ProjResult 
{
  glm::dvec2 point;
  bool is_inside;
};

// Calculate the signed area of the contour using the shoelace formula.
f32 calculate_signed_area(const std::vector<glm::dvec2>& contour);

// Calculate the bounding box for 2D points
BoundingBox2D calculate_bbox_2D(const std::vector<Segment>& segments);
BoundingBox2D calculate_bbox_2D(const std::vector<glm::dvec2>& points, const std::vector<u32>& cluster_indices);

// Returns the two long sides of the bounding box. 
// If the bounding box is wider than it is tall, it returns the top and bottom sides. 
// Otherwise, returns the left and right sides.
std::array<Segment, 2> get_long_sides_bbox2d(const BoundingBox2D& bbox);

// Calculates the bounding box for 3D points
BoundingBox3D calculate_bbox_3D(const std::vector<Vertex_PN>& vertices);

// Calculate the unit scale based on the geometry
f32 detect_unit_scale(f32 area_bbox);

// Normalizes all segment coordinates according to the drawing's measurement unit.
// Scales the start and end spatial coordinates of every segment in the collection by a conversion factor. 
void normalize_segments(f32 unit, std::vector<Segment>& segments);

// Snaps loose wall segment endpoints onto a unified topological vertex grid.
// Processes a collection of raw wall segments using a spatial hashing data structure to merge 
// coincident or near-coincident endpoints within a defined epsilon tolerance. This operation 
// repairs microscopic gaps common in CAD exports and transforms disconnected drawing lines into 
// a cohesive network of shared topological vertices and edges. Degenerate edges (where both 
// endpoints snap to the exact same vertex) are automatically filtered out.
std::vector<Edge> vertex_snapping(const std::vector<Segment> walls_segments,
                                  SpatialHash& hash);

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

// Calculates the orthogonal projection of a point onto a line segment.
// Computes where a given 2D point projects onto the line defined by two segment endpoints.
// It evaluates whether the projected point falls strictly inside the segment boundaries
ProjResult project_onto_segment(const glm::dvec2& p, const glm::dvec2& v1, const glm::dvec2& v2);

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
                                                 f32 eps);

// Connects and seals a gap in a wall (door or window opening).
// This function snaps the given gap coordinates to the nearest wall vertices using a Spatial Hash,
// finds the opposite wall vertices to account for wall thickness, handles non-colinear wall 
// segments via perpendicular projection, and inserts the closing edges with the specified layer type.
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


// Builds a triangulated face by iterating over a set of CDT triangles.
// Converts 2D polygonal triangle points into 3D vertices at a specified height,
// assigning a uniform normal vector to all vertices.
void build_triangulated_face(std::vector<Vertex_PN>& out_vertices,
                             std::vector<u32>& out_indices,
                             const std::vector<p2t::Triangle*> triangles,
                             f32 height,
                             const glm::vec3& normal);

// Extrudes a 2D face contour into a 3D quad-based wall segment between two heights.
// Creates four vertices (Bottom-Left, Bottom-Right, Top-Right, Top-Left) for each edge
// of the contour, calculates the outward-facing normal based on the edge direction,
// and pushes two triangles per edge to the index buffer.
void extrude_face(std::vector<Vertex_PN>& vertices, 
                  std::vector<u32>& out_indices,
                  f32 base_height,
                  f32 top_height,
                  const Face& face);

void center_mesh(std::vector<Vertex_PN>& vertices);