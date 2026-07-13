#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_float2.hpp>
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
enum class ThreadState { Idle, Running, WaitingConfirmation, Error };

using VertexId = u32;
constexpr VertexId INVALID_VERTEX_ID = std::numeric_limits<VertexId>::max();

enum class LayerType : i32
{ 
  NONE      = 0, 
  WALL      = 1,
  WINDOW    = 2,
  DOOR      = 3
};

struct Segment
{
  glm::dvec2 start, end;
  LayerType layer;
};

enum class FaceType : i32
{
  NONE    = 0, 
  FLOOR   = 1, 
  WINDOW  = 2, 
  DOOR    = 3, 
  WALL    = 4
};

struct Face
{
  std::vector<glm::dvec2> vertices;
  std::vector<LayerType> edge_layers;
  FaceType type;
};

struct Edge
{
  VertexId v1, v2;
  LayerType layer;
};

struct Vertex_PNT
{
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 text_coord;
};

struct BoundingBox2D
{
  glm::dvec2 min, max;

  auto calculate_area() { return (max.x - min.x) * (max.y - min.y); }
  
  // Check if a single point is inside the Bounding Box
  auto contains(glm::dvec2 p){ return (p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y); }
};

struct BoundingBox3D
{
  glm::vec3 min, max;
};

struct ProjResult 
{
  glm::dvec2 point; // Projected point (even if outside the segment)
  bool is_inside;   // True if projection lies strictly inside the segment
};

struct WallVertices
{
  VertexId B; // Start of the gap (snapped to gap_start)
  VertexId D; // End of the gap (snapped to gap_end)
  VertexId C; // Adjacent vertex to B along the wall
  VertexId E; // Adjacent vertex to D along the wall
};

enum class OpeningType { Door, Window };
struct OpeningInstance
{
  OpeningType type; // DOOR or WINDOW
  glm::vec3 center;
  f32 width;
  f32 height;
  f32 thickness;
  f32 rotation_z; // in radiants
};