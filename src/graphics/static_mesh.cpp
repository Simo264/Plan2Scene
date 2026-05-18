#include "static_mesh.hpp"

#include <cstddef>
#include <stdexcept>

StaticMesh::StaticMesh(const Vertex_PN* vertices, u32 nr_vertices, const u32* indices, u32 nr_indices) : 
	m_nr_vertices{ nr_vertices },
	m_nr_indices{ nr_indices }
{
	if(nr_vertices == 0 || nr_vertices == static_cast<u32>(-1))
		throw std::runtime_error("Invalid value of nr_vertices");
	
	m_vbo.create();
	m_vbo.allocate_storage(nr_vertices * sizeof(Vertex_PN), vertices, BufferUsageFlags::DynamicStorage);
	
	if(nr_indices != 0 && indices != nullptr)
	{
		m_ibo.create();
		m_ibo.allocate_storage(nr_indices * sizeof(u32), indices, BufferUsageFlags::DynamicStorage);	
	}	
	
	m_vao.create();
  // Attribute 0: position(xyz)
  m_vao.set_attrib_format_float(0, 3, VertexAttribType::Float, false, offsetof(Vertex_PN, position));
  // Attribute 1: normal(x,y,z)
  m_vao.set_attrib_format_float(1, 3, VertexAttribType::Float, true, offsetof(Vertex_PN, normal));
  m_vao.attach_vertex_buffer(0, m_vbo, 0, sizeof(Vertex_PN));
  
  m_vao.link_attrib(0, 0);
  m_vao.link_attrib(1, 0); 
  m_vao.enable_attrib(0);
  m_vao.enable_attrib(1);
  
  if(m_ibo.is_valid())
  	m_vao.attach_index_buffer(m_ibo);
}

StaticMesh::~StaticMesh()
{
	m_vbo.destroy();
	m_ibo.destroy();
	m_vao.destroy();
}