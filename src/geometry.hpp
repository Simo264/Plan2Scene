#pragma once

#include "spatial_hashing.hpp"
#include "types.hpp"

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

// Reconstructs door geometries and seamlessly integrates them into the wall topology.
// Iterates through the door segments to establish their closed topological boundaries. 
// For each door, it finds the adjacent inner wall vertices using directional lookups. 
// The function performs cross-projections between the opposite wall endpoints. 
// If an overhang is detected, it dynamically splits the longer wall edge, 
// inserts a new aligned vertex, and accurately seals the door as a perfect rectangle.
void doors_reconstruction(std::vector<Segment>& doors,
                          SpatialHash& hash,
                          std::vector<Edge>& edges);

void build_triangulated_face(std::vector<Vertex_PN>& out_vertices,
                             std::vector<u32>& out_indices,
                             const std::vector<p2t::Triangle*> triangles,
                             f32 height,
                             const glm::vec3& normal);

void extrude_face(std::vector<Vertex_PN>& vertices, 
                  std::vector<u32>& out_indices,
                  f32 base_height,
                  f32 top_height,
                  const Face& face);

void center_mesh(std::vector<Vertex_PN>& vertices);