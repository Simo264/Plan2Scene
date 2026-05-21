#pragma once

#include <cstdint>
#include <cstddef>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_float3.hpp>

// ----------------------------------------------------------------------------
// Byte type
// ----------------------------------------------------------------------------
using byte = std::byte;
using uchar = unsigned char;

// ----------------------------------------------------------------------------
// Unsigned integer types
// ----------------------------------------------------------------------------
using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

// ----------------------------------------------------------------------------
// Signed integer types
// ----------------------------------------------------------------------------
using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

// ----------------------------------------------------------------------------
// Floating point types
// ----------------------------------------------------------------------------
using f32 = float;
using f64 = double;

 
// ----------------------------------------------------------------------------
// Geometric types
// ----------------------------------------------------------------------------

enum class LayerType : i32
{ 
  NONE = 0, WALL = 1, WINDOW = 2, DOOR = 3
};

struct Segment
{
  glm::dvec2 p1, p2;
  LayerType layer;
};

struct Vertex_PN
{
  glm::vec3 position;
  glm::vec3 normal;
};

struct BoundingBox
{
  glm::vec3 min;
  glm::vec3 max;
};

using VertexId = u32;

struct GraphEdge
{
  VertexId v1, v2;
  LayerType layer;
};
