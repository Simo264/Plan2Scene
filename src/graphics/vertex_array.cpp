#include "vertex_array.hpp"

#include <glad/gl.h>
#include <print>

void VerteArray::create()
{
  glCreateVertexArrays(1, &m_id);
  if(!is_valid())
    std::println("Failed to create vertex array (id: {})", m_id);
}

void VerteArray::destroy()
{
  glDeleteVertexArrays(1, &m_id);
}

bool VerteArray::is_valid() const
{
  return m_id != 0 && glIsVertexArray(m_id) == GL_TRUE;
}

void VerteArray::bind() const
{
  glBindVertexArray(m_id);
}

void VerteArray::unbind() const
{
  glBindVertexArray(0);
}

void VerteArray::enable_attrib(u32 index) const
{
  glEnableVertexArrayAttrib(m_id, index);
}

void VerteArray::disable_attrib(u32 index) const
{
  glDisableVertexArrayAttrib(m_id, index);
}

void VerteArray::set_attrib_format_float(u32 attrindex, i32 size, VertexAttribType type, bool normalized, u32 offset) const
{
  glVertexArrayAttribFormat(m_id, attrindex, size, static_cast<i32>(type), normalized, offset);
}

void VerteArray::set_attrib_format_int(u32 attrindex, i32 size, VertexAttribType type, u32 offset) const
{
  glVertexArrayAttribIFormat(m_id, attrindex, size, static_cast<i32>(type), offset);
}

void VerteArray::set_attrib_format_long(u32 attrindex, i32 size, VertexAttribType type, u32 offset) const
{
  glVertexArrayAttribLFormat(m_id, attrindex, size, static_cast<i32>(type), offset);
}

void VerteArray::attach_index_buffer(Buffer buffer) const
{
  glVertexArrayElementBuffer(m_id, buffer.id());
}

void VerteArray::detach_index_buffer() const
{
  glVertexArrayElementBuffer(m_id, 0);
}

void VerteArray::attach_vertex_buffer(u32 bindingindex, Buffer buffer, i32 offset, i32 stride) const
{
  glVertexArrayVertexBuffer(m_id, bindingindex, buffer.id(), offset, stride);
}

void VerteArray::link_attrib(u32 attrindex, u32 bindingindex) const
{
  glVertexArrayAttribBinding(m_id, attrindex, bindingindex);
}