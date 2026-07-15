#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <array>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <poly2tri/common/shapes.h>

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
// Thread state enum type for the worker thread 
// ----------------------------------------------------------------------------
enum class ThreadState { Idle, Running, WaitingConfirmation, Error };

// ----------------------------------------------------------------------------
// Geometric types
// ----------------------------------------------------------------------------
using VertexId = u32;
constexpr VertexId INVALID_VERTEX_ID = std::numeric_limits<VertexId>::max();

enum class SegmentLayer : i32 { None=0, Wall=1, Window=2, Door=3 };
enum class FaceType : i32 { None=0, Floor=1, Window=2, Door=3, Wall=4 };
enum class OpeningType { Door=0, Window=1 };

struct Segment
{
  glm::dvec2 start, end;
  SegmentLayer layer;
};

struct Edge
{
  VertexId v1, v2;
  SegmentLayer layer;
};

struct Vertex_PNT
{
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 text_coord;
};

class Face
{
public:
  Face() : type{ FaceType::None } {}

  std::vector<glm::dvec2> vertices;
  std::vector<SegmentLayer> edge_layers;
  FaceType type;

  // Calculate the center of the face
  glm::dvec2 calculate_center() const;

  // Triangulates a polygonal face
  // void triangulate(std::vector<Vertex_PNT>& out_vertices,
  //                  std::vector<u32>& out_indices,
  //                  f32 height,
  //                  f32 texture_scaling,
  //                  bool facing_up) const;

  // Extrudes a 2D face into a 3D quad-based.
  // Creates four vertices for each edge of the contour, calculates the outward-facing normal based on the edge direction
  void extrude(std::vector<Vertex_PNT>& out_vertices,
               std::vector<u32>& out_indices,
               f32 base_height, 
               f32 top_height, 
               f32 texture_scaling = 1.0f) const;

private:
  // void perform_triangulation(std::vector<Vertex_PNT>& out_vertices,
  //                            std::vector<u32>& out_indices,
  //                            const std::vector<p2t::Triangle*> triangles,
  //                            f32 height,
  //                            f32 texture_scaling,
  //                            bool facing_up) const;

  // void ensure_winding_matches_normal(Vertex_PNT& v0,
  //                                    Vertex_PNT& v1, 
  //                                    Vertex_PNT& v2, 
  //                                    glm::vec3 desired_normal) const;
};

struct WallVertices
{
  VertexId B; // Start of the gap (snapped to gap_start)
  VertexId D; // End of the gap (snapped to gap_end)
  VertexId C; // Adjacent vertex to B along the wall
  VertexId E; // Adjacent vertex to D along the wall
};

struct OpeningInstance
{
  OpeningType type;
  glm::vec3 center;
  f32 width;
  f32 height;
  f32 thickness;
  f32 rotation_z; // in radiants
};

struct BoundingBox2D
{
  glm::dvec2 min, max;

  static BoundingBox2D calculate_from_contour(const std::vector<glm::dvec2>& polyline);
  static BoundingBox2D calculate_from_face(const Face& face);
  static BoundingBox2D calculate_from_segments(const std::vector<Segment>& segments);
  static BoundingBox2D calculate_from_cluster(const std::vector<glm::dvec2>& points, const std::vector<u32>& cluster_indices);

  // calculate the area of the bounding box
  f64 calculate_area() const;
    // Check if a single point is inside the Bounding Box
  bool contains(glm::dvec2 p) const;

  // Returns the two long sides of the bounding box.
  std::array<Segment, 2> get_long_sides() const;
};

struct BoundingBox3D
{
  glm::vec3 min, max;

  // Calculates the bounding box for 3D points
  static BoundingBox3D calculate_from_vertices(const std::vector<Vertex_PNT>& vertices);
};

