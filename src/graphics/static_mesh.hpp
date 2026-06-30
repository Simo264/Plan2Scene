#pragma once

#include "vertex_array.hpp"

enum class MaterialType{ Floor, Wall };

struct PrimitiveRange 
{
  u32 index_offset; // starting index in the IBO (not bytes)
  u32 index_count;
  MaterialType material;
};

class StaticMesh 
{
public:
	StaticMesh(const Vertex_PNT* vertices, u32 nr_vertices, const u32* indices, u32 nr_indices);
	// clear VRAM memory
  ~StaticMesh();
	
	u32 nr_vertices;
	u32 nr_indices;
	
	Buffer vbo;
	Buffer ibo;
	VerteArray vao;
};