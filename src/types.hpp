#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
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
  glm::dvec2 p1{ 0.0 }, p2{ 0.0 };
  LayerType layer { LayerType::NONE };
};

struct Polyline
{
  std::vector<glm::dvec2> points;
  bool closed{ false };
};

struct Vertex_PN
{
  glm::vec3 position{ 0.f };
  glm::vec3 normal{ 0.f };
};

struct BoundingBox
{
  glm::vec3 min{ 0.f };  // the bottom left corner
  glm::vec3 max{ 0.f };  // the top right corner
};

using VertexId = u32;

struct GraphVertex
{
  glm::dvec2 position;
};

struct GraphEdge
{
  VertexId v1, v2;
  LayerType layer{ LayerType::NONE };
};



