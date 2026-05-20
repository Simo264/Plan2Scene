#pragma once

#include "types.hpp"

#include <poly2tri/sweep/cdt.h>
#include <vector>

// Calculate the signed area of the contour using the shoelace formula.
auto calculate_signed_area(const Polyline& contour) -> f32;

// Calculates the bounding box for 2D points. The Z-coordinate is set to 0.0f by default.
auto calculate_bounding_box(const std::vector<glm::dvec2>& points) -> BoundingBox;
// Calculates the bounding box for 3D points
auto calculate_bounding_box(const std::vector<Vertex_PN>& vertices) -> BoundingBox;

// Calculate the unit scale based on the geometry
auto detect_unit_scale(const std::vector<glm::dvec2>& points) -> f32;

auto compute_polygon_offsetting(const std::vector<glm::dvec2>& inner_points, 
                                f32 thickness) -> std::vector<glm::dvec2>;

void build_floor(std::vector<Vertex_PN>& out_vertices,
                 std::vector<u32>& out_indices,
                 const std::vector<p2t::Triangle*> floor_triangles);

void build_ceil(std::vector<Vertex_PN>& out_vertices, 
                std::vector<u32>& out_indices,
                f32 H,
                const std::vector<p2t::Triangle*> triangle_list);


void extrude_walls(std::vector<Vertex_PN>& vertices, 
                   std::vector<u32>& out_indices,
                   f32 H,
                   const std::vector<glm::dvec2>& inner_points,
                   const std::vector<glm::dvec2>& outer_points);
                
void build_wall_top_cap(std::vector<Vertex_PN>& out_vertices,
                        std::vector<u32>& out_indices,
                        f32 H,
                        const std::vector<glm::dvec2>& inner_points,
                        const std::vector<glm::dvec2>& outer_points);

