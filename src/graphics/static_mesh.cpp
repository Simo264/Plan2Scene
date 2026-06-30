#include "static_mesh.hpp"

#include <cstddef>
#include <stdexcept>

StaticMesh::StaticMesh(const Vertex_PNT* vertices, u32 nr_vertices, const u32* indices, u32 nr_indices) : 
	nr_vertices{ nr_vertices },
	nr_indices{ nr_indices }
{
	if(nr_vertices == 0 || vertices == nullptr)
		throw std::runtime_error("Invalid vertices data for StaticMesh!");

	if(nr_indices == 0 || indices == nullptr)
	   throw std::runtime_error("Invalid indices data for StaticMesh!");
	
	vbo.create();
	vbo.allocate_storage(nr_vertices * sizeof(Vertex_PNT), vertices, BufferUsageFlags::DynamicStorage);
	
	ibo.create();
	ibo.allocate_storage(nr_indices * sizeof(u32), indices, BufferUsageFlags::DynamicStorage);	

	vao.create();
  // Attribute 0: position(xyz)
  vao.set_attrib_format_float(0, 3, VertexAttribType::Float, false, offsetof(Vertex_PNT, position));
  // Attribute 1: normal(x,y,z)
  vao.set_attrib_format_float(1, 3, VertexAttribType::Float, true, offsetof(Vertex_PNT, normal));
  // Attribute 2: text_coord(u,v)
  vao.set_attrib_format_float(2, 2, VertexAttribType::Float, false, offsetof(Vertex_PNT, text_coord));
  
  vao.link_attrib(0, 0);
  vao.link_attrib(1, 0); 
  vao.link_attrib(2, 0); 
  vao.enable_attrib(0);
  vao.enable_attrib(1);
  vao.enable_attrib(2);

  vao.attach_vertex_buffer(0, vbo, 0, sizeof(Vertex_PNT));
 	vao.attach_index_buffer(ibo);
}

StaticMesh::~StaticMesh()
{
	vbo.destroy();
	ibo.destroy();
	vao.destroy();
}