#pragma once

#include "types.hpp"

#include <poly2tri/sweep/cdt.h>
#include <vector>
#include <span>

// Calculate the signed area of the contour using the shoelace formula.
f32 calculate_signed_area(const std::vector<glm::dvec2>& contour);

// Calculate the bounding box for 2D points
BoundingBox2D calculate_bbox_2D(std::span<const Segment> segments);

// Calculates the bounding box for 3D points
BoundingBox3D calculate_bbox_3D(const std::vector<Vertex_PN>& vertices);

// Check if a single point is inside the Bounding Box
bool is_point_inside_bbox(const BoundingBox2D& bbox, const glm::dvec2& p);

// Calculate the unit scale based on the geometry
f32 detect_unit_scale(const std::vector<Segment>& wall_segments);

void normalize_segments(f32 unit, std::vector<Segment>& segments);

std::array<VertexId, 2> find_neighboors(VertexId vertex, 
                                        const std::vector<Edge>& edges);

VertexId get_adjacent_vertex(const glm::dvec2& wall_dir, 
                             VertexId vertex_id,
                             const std::array<VertexId, 2>& vertex_neighbors, 
                             const std::vector<glm::dvec2>& vertices);

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