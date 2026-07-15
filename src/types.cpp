#include "types.hpp"

#include <algorithm>
#include <numeric>

#include <glm/common.hpp>
#include <glm/geometric.hpp>


#include <poly2tri/sweep/cdt.h>

// =========================
// BoundingBox2D
// =========================

BoundingBox2D BoundingBox2D::calculate_from_contour(const std::vector<glm::dvec2>& polyline)
{
  if (polyline.empty()) 
    return BoundingBox2D{ glm::dvec2{0.0, 0.0}, glm::dvec2{0.0, 0.0} };

  auto min_x = std::numeric_limits<f64>::max();
  auto min_y = std::numeric_limits<f64>::max();
  auto max_x = -std::numeric_limits<f64>::max();
  auto max_y = -std::numeric_limits<f64>::max();
  for (const auto& p : polyline) 
  {
    min_x = std::min(min_x, p.x);
    min_y = std::min(min_y, p.y);
    max_x = std::max(max_x, p.x);
    max_y = std::max(max_y, p.y);
  }

  return BoundingBox2D{
    .min = glm::dvec2{ min_x, min_y },
    .max = glm::dvec2{ max_x, max_y }
  };
}

BoundingBox2D BoundingBox2D::calculate_from_face(const Face& face)
{
  const auto& polyline = face.vertices;
  return BoundingBox2D::calculate_from_contour(polyline);
}

BoundingBox2D BoundingBox2D::calculate_from_segments(const std::vector<Segment>& segments)
{
  auto min_x =  std::numeric_limits<f64>::max();
  auto min_y =  std::numeric_limits<f64>::max();
  auto max_x = -std::numeric_limits<f64>::max();
  auto max_y = -std::numeric_limits<f64>::max();
  for (const auto& seg : segments)
  {
    min_x = std::min({min_x, seg.start.x, seg.end.x});
    min_y = std::min({min_y, seg.start.y, seg.end.y});
    max_x = std::max({max_x, seg.start.x, seg.end.x});
    max_y = std::max({max_y, seg.start.y, seg.end.y});
  }
  return BoundingBox2D{
    .min = glm::dvec2{ min_x, min_y },
    .max = glm::dvec2{ max_x, max_y }
  };
}

BoundingBox2D BoundingBox2D::calculate_from_cluster(const std::vector<glm::dvec2>& points, const std::vector<u32>& cluster_indices)
{
  auto min_x = std::numeric_limits<f64>::max();
  auto min_y = std::numeric_limits<f64>::max();
  auto max_x = -std::numeric_limits<f64>::max();
  auto max_y = -std::numeric_limits<f64>::max();
  for (int idx : cluster_indices) 
  {
    const auto& p = points[idx];
    min_x = std::min(min_x, p.x);
    min_y = std::min(min_y, p.y);
    max_x = std::max(max_x, p.x);
    max_y = std::max(max_y, p.y);
  }

  return BoundingBox2D{
    .min = glm::dvec2{ min_x, min_y },
    .max = glm::dvec2{ max_x, max_y }
  };
}

f64 BoundingBox2D::calculate_area() const
{ 
  return (max.x - min.x) * (max.y - min.y); 
}

bool BoundingBox2D::contains(glm::dvec2 p) const
{ 
  return (p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y); 
}

std::array<Segment, 2> BoundingBox2D::get_long_sides() const
{
  auto dx = max.x - min.x;
  auto dy = max.y - min.y;
  if (dx > dy) 
  {
    // The X-axis is dominant: the long sides are horizontal
    auto bottom = Segment{
      .start = glm::dvec2(min.x, min.y),
      .end   = glm::dvec2(max.x, min.y),
      .layer = SegmentLayer::None
    };
    auto top = Segment{
      .start = glm::dvec2(min.x, max.y),
      .end   = glm::dvec2(max.x, max.y),
      .layer = SegmentLayer::None
    };
    return { bottom, top };
  } 
  else 
  {
    // The Y-axis is dominant: the long sides are vertical
    auto left = Segment{
      .start = glm::dvec2(min.x, min.y),
      .end   = glm::dvec2(min.x, max.y),
      .layer = SegmentLayer::None
    };
    auto right = Segment{
      .start = glm::dvec2(max.x, min.y),
      .end   = glm::dvec2(max.x, max.y),
      .layer = SegmentLayer::None
    };
    return { left, right };
  }
}

// =========================
// BoundingBox3D
// =========================

BoundingBox3D BoundingBox3D::calculate_from_vertices(const std::vector<Vertex_PNT>& vertices)
{
  auto min = vertices.front().position;
  auto max = min;
  for (const auto& p : vertices) 
  {
    min = glm::min(min, p.position);
    max = glm::max(max, p.position);
  }
  return BoundingBox3D{ min, max };
}

// =========================
// Face
// =========================

glm::dvec2 Face::calculate_center() const
{
  auto sum = std::accumulate(vertices.begin(), vertices.end(), glm::dvec2(0.0));
  return sum / static_cast<double>(vertices.size());
}

void Face::extrude(std::vector<Vertex_PNT>& out_vertices,
                   std::vector<u32>& out_indices,
                   f32 base_height, 
                   f32 top_height, 
                   f32 texture_scaling) const
{
  constexpr glm::vec3 up(0.0f, 1.0f, 0.0f);
  const auto& contour = vertices; 
  auto v_bottom_uv = base_height / texture_scaling; 
  auto v_top_uv = top_height / texture_scaling; 
  for (auto i = 0u; i < contour.size(); ++i)  
  { 
    auto p1 = contour[i]; 
    auto p2 = contour[(i + 1) % contour.size()]; 
    auto edge = glm::vec3(f32(p2.x - p1.x), 0.0f, f32(p2.y - p1.y)); 
    auto edge_len = glm::length(edge); 
    auto normal = glm::normalize(glm::cross(up, edge)); 
    auto u0 = 0.0f; 
    auto u1 = edge_len / texture_scaling; 

    auto BL = Vertex_PNT{ {f32(p1.x), base_height, f32(p1.y)}, normal, {u0, v_bottom_uv} }; 
    auto BR = Vertex_PNT{ {f32(p2.x), base_height, f32(p2.y)}, normal, {u1, v_bottom_uv} }; 
    auto TR = Vertex_PNT{ {f32(p2.x), top_height, f32(p2.y)}, normal, {u1, v_top_uv} }; 
    auto TL = Vertex_PNT{ {f32(p1.x), top_height, f32(p1.y)}, normal, {u0, v_top_uv} }; 

    auto tri_normal = glm::cross(BR.position - BL.position, TR.position - BL.position);
    auto winding_ok = glm::dot(tri_normal, normal) >= 0.0f;
    
    auto base = static_cast<u32>(out_vertices.size()); 
    out_vertices.push_back(BL); 
    out_vertices.push_back(BR); 
    out_vertices.push_back(TR); 
    out_vertices.push_back(TL); 
    if (winding_ok)
    {
      out_indices.push_back(base + 0);
      out_indices.push_back(base + 1);
      out_indices.push_back(base + 2);
      out_indices.push_back(base + 0);
      out_indices.push_back(base + 2);
      out_indices.push_back(base + 3);
    }
    else
    {
      out_indices.push_back(base + 0);
      out_indices.push_back(base + 2);
      out_indices.push_back(base + 1);
      out_indices.push_back(base + 0);
      out_indices.push_back(base + 3);
      out_indices.push_back(base + 2);
    } 
  }
}

// void Face::ensure_winding_matches_normal(Vertex_PNT& v0, 
//                                          Vertex_PNT& v1, 
//                                          Vertex_PNT& v2, 
//                                          glm::vec3 desired_normal) const
// {
//   auto geometric_normal = glm::cross(v1.position - v0.position, v2.position - v0.position);
//   if (glm::dot(geometric_normal, desired_normal) < 0.0f)
//     std::swap(v1, v2);
// }

// void Face::perform_triangulation(std::vector<Vertex_PNT>& out_vertices,
//                                  std::vector<u32>& out_indices,
//                                  const std::vector<p2t::Triangle*> triangles,
//                                  f32 height,
//                                  f32 texture_scaling,
//                                  bool facing_up) const
// {
//   auto face_bbox = BoundingBox2D::calculate_from_contour(this->vertices);

//   auto desired_normal = facing_up ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(0.0f, -1.0f, 0.0f);
//   for (const auto& tri : triangles) 
//   { 
//     auto verts = std::array<Vertex_PNT, 3>{};
//     for (auto i = 0; i < 3; ++i)  
//     {  
//       auto p = tri->GetPoint(i); 
//       auto v = Vertex_PNT{}; 
//       v.position.x = static_cast<f32>(p->x); 
//       v.position.y = height; 
//       v.position.z = static_cast<f32>(p->y); 
//       v.normal = desired_normal;
//       v.text_coord.x = (v.position.x - face_bbox.min.x) / texture_scaling; 
//       v.text_coord.y = (v.position.z - face_bbox.min.y) / texture_scaling; 
//       verts[i] = v;
//     }

//     ensure_winding_matches_normal(verts[0], verts[1], verts[2], desired_normal);

//     for (const auto& v : verts)
//     {
//       auto idx = static_cast<u32>(out_vertices.size()); 
//       out_vertices.push_back(v); 
//       out_indices.push_back(idx); 
//     }
//   } 
// }

// void Face::triangulate(std::vector<Vertex_PNT>& out_vertices,
//                        std::vector<u32>& out_indices,
//                        f32 height,
//                        f32 texture_scaling,
//                        bool facing_up) const
// {
//   auto polyline = vertices;
//   auto p2t_points = std::vector<p2t::Point>{};
//   auto p2t_ptr_points = std::vector<p2t::Point*>{};
//   p2t_points.reserve(polyline.size());
//   p2t_ptr_points.reserve(polyline.size());
//   for (const auto& p : polyline)
//   {
//     p2t_points.emplace_back(p2t::Point{ p.x, p.y });
//     p2t_ptr_points.push_back(&p2t_points.back());
//   }
  
//   auto cdt = p2t::CDT{ p2t_ptr_points };
//   cdt.Triangulate();
//   auto triangles = cdt.GetTriangles();

//   perform_triangulation(out_vertices, out_indices, triangles, texture_scaling, height, facing_up);
// }
