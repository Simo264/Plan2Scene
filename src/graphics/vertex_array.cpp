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


void VerteArray::enable_attrib(std::uint32_t index) const
{
  glEnableVertexArrayAttrib(m_id, index);
}

void VerteArray::disable_attrib(std::uint32_t index) const
{
  glDisableVertexArrayAttrib(m_id, index);
}

void VerteArray::set_attrib_format_float(std::uint32_t attrindex, std::int32_t size, VertexAttribType type, bool normalized, std::uint32_t offset) const
{
  glVertexArrayAttribFormat(m_id, attrindex, size, static_cast<std::int32_t>(type), normalized, offset);
}

void VerteArray::set_attrib_format_int(std::uint32_t attrindex, std::int32_t size, VertexAttribType type, std::uint32_t offset) const
{
  glVertexArrayAttribIFormat(m_id, attrindex, size, static_cast<std::int32_t>(type), offset);
}

void VerteArray::set_attrib_format_long(std::uint32_t attrindex, std::int32_t size, VertexAttribType type, std::uint32_t offset) const
{
  glVertexArrayAttribLFormat(m_id, attrindex, size, static_cast<std::int32_t>(type), offset);
}

void VerteArray::attach_index_buffer(Buffer buffer) const
{
  glVertexArrayElementBuffer(m_id, buffer.id());
}

void VerteArray::detach_index_buffer() const
{
  glVertexArrayElementBuffer(m_id, 0);
}

void VerteArray::attach_vertex_buffer(std::uint32_t bindingindex, Buffer buffer, std::int32_t offset, std::int64_t stride) const
{
  glVertexArrayVertexBuffer(m_id, bindingindex, buffer.id(), offset, stride);
}

void VerteArray::link_attrib(std::uint32_t attrindex, std::uint32_t bindingindex) const
{
  glVertexArrayAttribBinding(m_id, attrindex, bindingindex);
}