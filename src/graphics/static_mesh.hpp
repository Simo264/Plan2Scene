#pragma once

#include "../geometry.hpp"
#include "vertex_array.hpp"

/**
 * Triangle meshes are a common representation for surfaces. The minimum
 * information required for a triangle mesh is a set of vertex positions and a
 * set of triangles (each triangle is defined by three vertex indices). Real
 * applications commonly store additional attributes per-vertex, per-edge, or
 * per-face — for example normals, texture coordinates, material IDs, or any
 * parameter that varies across the surface.
 *
 * Storing each triangle independently leads to duplicated vertex data when
 * vertices are shared between triangles. An indexed (shared-vertex)
 * representation avoids this duplication by storing a contiguous array of
 * unique vertices and an index buffer that references those vertices for each
 * triangle:
 *
 *   Vertex vertices[NUM_VERTICES];
 *   uint32_t indices[NUM_TRIANGLES][3];
 *
 * For static geometry, triangle strips and triangle fans are alternative
 * compact encodings that can further reduce storage for certain meshes. This
 * class is intended as the place to represent static, shared-vertex triangle
 * geometry.
 */
class StaticMesh 
{
public:
	StaticMesh(const Vertex* vertices, u32 nr_vertices, const u32* indices, u32 nr_indices);
	// clear VRAM memory
  ~StaticMesh();
	
	auto nr_vertices() const { return m_nr_vertices; }
	auto nr_indices() const { return m_nr_indices; }
	auto vao() const { return m_vao; }
	
	auto is_drawable() const { return m_vao.is_valid() && m_vbo.is_valid(); }
private:
	u32 m_nr_vertices;
	u32 m_nr_indices;
	
	Buffer m_vbo;
	Buffer m_ibo;
	VerteArray m_vao;
};