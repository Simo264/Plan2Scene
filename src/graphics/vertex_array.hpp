#pragma once

#include "buffer.hpp"

using VertexArrayID = u32;

enum class VertexAttribType : i32
{
  Byte                        = 0x1400, // GL_BYTE
  UnsignedByte                = 0x1401, // GL_UNSIGNED_BYTE
  Short                       = 0x1402, // GL_SHORT
  UnsignedShort               = 0x1403, // GL_UNSIGNED_SHORT
  Int                         = 0x1404, // GL_INT
  UnsignedInt                 = 0x1405, // GL_UNSIGNED_INT
  Float                       = 0x1406, // GL_FLOAT
  Double                      = 0x140A, // GL_DOUBLE
  HalfFloat                   = 0x140B, // GL_HALF_FLOAT
  Fixed                       = 0x140C, // GL_FIXED 

  Int_2_10_10_10_Rev          = 0x8D9F, // GL_INT_2_10_10_10_REV
  UnsignedInt_2_10_10_10_Rev  = 0x8368, // GL_UNSIGNED_INT_2_10_10_10_REV 
  UnsignedInt_10F_11F_11F_Rev = 0x8C3B, // GL_UNSIGNED_INT_10F_11F_11F_REV 
};

class VerteArray
{
public:
  VerteArray() : m_id{ 0 } {}
 
  // Create vertex array object
  void create();
  // Destroy vertex array object
  void destroy();
  
  // Bind vertex array object
  void bind() const;
  // Unbind vertex array object
  void unbind() const;
  
  // Enable a generic vertex attribute array
  void enable_attrib(u32 index) const;
  // Disable a generic vertex attribute array
  void disable_attrib(u32 index) const;
  
  // Define the format of a generic vertex attribute. The vertex format information above tells OpenGL how to interpret the data.
  // The format says how big each vertex is in bytes and how to convert it into the values that the attribute in the vertex shader receives.
  // The `attrindex` parameter specifies which attribute to define.
  // The `size` parameter specifies the number of components per attribute (1, 2, 3, or 4).
  // The `type` parameter specifies the data type of each component (GL_FLOAT, GL_INT, or GL_LONG).
  // The `normalized` parameter specifies whether to normalize fixed-point data.
  // The `offset` parameter specifies the byte offset of the first component in the array.
  void set_attrib_format_float(u32 attrindex, i32 size, VertexAttribType type, bool normalized, u32 offset) const;
  void set_attrib_format_int(u32 attrindex, i32 size, VertexAttribType type, u32 offset) const;
  void set_attrib_format_long(u32 attrindex, i32 size, VertexAttribType type, u32 offset) const;
  
  // Specifies the buffer object to use for the element array buffer binding.
  void attach_index_buffer(Buffer buffer) const;
  // Any existing element array buffer binding to vertex array object is removed.
  void detach_index_buffer() const;
  
  // Bind a buffer to a vertex buffer bind point.
  // The `bindingindex` is the index of the vertex buffer binding point to which to bind the buffer.
  // The `offset` is the offset (in bytes) of the first element of the buffer.
  // The `stride` is the distance (in bytes) between elements within the buffer. 
  void attach_vertex_buffer(u32 bindingindex, Buffer buffer, i32 offset, std::int64_t stride) const;
  
  // Associate a vertex attribute and a vertex buffer binding for a vertex array object.
  // The `attrindex` is the index of the attribute to associate with a vertex buffer binding.
  // The `bindingindex` is the index of the vertex buffer binding with which to associate the generic vertex attribute.
  void link_attrib(u32 attrindex, u32 bindingindex) const;
  
  // Check if vertex array object is valid
  bool is_valid() const;
  
  auto id() const { return m_id; }

private:
  VertexArrayID m_id;
};