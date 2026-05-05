#pragma once

#include "geometry.hpp"
#include "poly2tri/common/shapes.h"

#if 0
// Appends a single triangle (3 vertices + 3 sequential indices) to the mesh.
// Note: this function does not check for duplicate vertices. It is the caller's responsibility to 
// ensure that vertices are not duplicated if they should be shared across triangles. 
void add_triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, IndexedMesh& mesh);

// Converts poly2tri output triangles into the floor of the 3D room mesh.
// Places all triangles at z = 0 with an upward-facing normal (0, 0, 1).
// Important: this function assumes the contour was already oriented counter-clockwise before triangulation.
void create_floor(const std::vector<p2t::Triangle*>& triangles, IndexedMesh& mesh);

// Creates the ceiling by vertically offsetting the floor triangles.
// Copies the same triangulation used for the floor, shifts all vertices to z = H,
// and reverses the winding order (v0, v2, v1 instead of v0, v1, v2) so that
// normals point downward (0, 0, -1), facing the room interior.
void create_ceiling(const std::vector<p2t::Triangle*>& triangles, float H, IndexedMesh& mesh);

// Generates vertical wall quads by extruding each edge of the closed contour upward.
// For each consecutive pair of vertices (v[i], v[i+1]) (including the closing
// edge from the last vertex back to the first) a vertical quad is created from
// z = 0 to z = H. Each quad consists of two triangles.
// 
// The outward-facing normal for each wall quad is computed as the 2D edge
// direction rotated 90° clockwise, which yields the correct exterior normal
// when the input polygon is wound counter-clockwise.
void extrude_walls(const Polyline& polyline, float H, IndexedMesh& mesh);
#endif