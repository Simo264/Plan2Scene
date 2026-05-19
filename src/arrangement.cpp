#include "arrangement.hpp"
#include "spatial_hashing.hpp"
#include "types.hpp"

#include <glm/geometric.hpp>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Arrangement_2.h>
#include <CGAL/Arr_naive_point_location.h>
#include <CGAL/Arr_walk_along_line_point_location.h>
#include <CGAL/Arr_non_caching_segment_traits_2.h>
#include <CGAL/Arr_observer.h>

using Kernel = CGAL::Exact_predicates_exact_constructions_kernel;
using Point_2 = Kernel::Point_2;
using Segment_2 = Kernel::Segment_2;
using Traits = CGAL::Arr_segment_traits_2<Kernel>;
using Arrangement = CGAL::Arrangement_2<Traits>;
using Vertex_handle = Arrangement::Vertex_handle;
using Halfedge_handle = Arrangement::Halfedge_handle;

static auto glm_to_cgal(const glm::dvec2& p) { return Point_2(p.x, p.y); }
static auto CGAL_to_glm(const Point_2& p) { return glm::dvec2(CGAL::to_double(p.x()), CGAL::to_double(p.y())); }

void resolve_tjunctions(std::vector<GraphVertex>& vertices, std::vector<GraphEdge>& edges)
{
  struct RawSegment { Segment_2 seg; LayerType layer; };

  // 1. Collect original segments
  std::vector<RawSegment> raw_segments;
  raw_segments.reserve(edges.size());
  for (const auto& e : edges)
  {
      const auto& p1 = vertices[e.v1].position;
      const auto& p2 = vertices[e.v2].position;

      if (p1 == p2)
        continue;

      raw_segments.push_back(RawSegment{ 
        .seg=Segment_2{ glm_to_cgal(p1), glm_to_cgal(p2) },
        .layer=e.layer
      });
  }


  // 2. Build arrangement (CGAL resolves T-junctions here)
  Arrangement arr;

  std::vector<Segment_2> cgal_segments;
  cgal_segments.reserve(raw_segments.size());
  for (const auto& rs : raw_segments)
    cgal_segments.push_back(rs.seg);

  CGAL::insert(arr, cgal_segments.begin(), cgal_segments.end());

  // 3. Export vertices
  std::vector<GraphVertex> new_vertices;
  std::unordered_map<Arrangement::Vertex_const_handle, VertexId> vertex_map;
  new_vertices.reserve(arr.number_of_vertices());
  for (auto vit = arr.vertices_begin(); vit != arr.vertices_end(); ++vit)
  {
      VertexId id = static_cast<VertexId>(new_vertices.size());
      new_vertices.push_back(GraphVertex{ 
        .position=CGAL_to_glm(vit->point())
      });
      vertex_map.emplace(vit, id);
  }


  // 4. Export subdivided edges
  std::vector<GraphEdge> new_edges;
  new_edges.reserve(arr.number_of_edges());

  for (auto eit = arr.edges_begin();eit != arr.edges_end();++eit)
  {
      const auto src = eit->source()->point();
      const auto tgt = eit->target()->point();
      if (src == tgt)
        continue;

  
      // Recover layer:
      // find original segment that contains both endpoints
      LayerType layer = LayerType::NONE;
      for (const auto& rs : raw_segments)
      {
        const auto& s = rs.seg;

        bool src_on_segment = CGAL::collinear(s.source(), s.target(), src) &&
            CGAL::collinear_are_ordered_along_line(s.source(), src, s.target());

        if (!src_on_segment)
          continue;

        bool tgt_on_segment = CGAL::collinear(s.source(), s.target(), tgt) &&
            CGAL::collinear_are_ordered_along_line(s.source(),tgt,s.target());

        if (!tgt_on_segment)
          continue;

        layer = rs.layer;
        break;
      }

      if (layer == LayerType::NONE)
        continue;

      VertexId v1 = vertex_map.at(eit->source());
      VertexId v2 = vertex_map.at(eit->target());

      if (v1 == v2)
        continue;

      new_edges.push_back(GraphEdge{ v1, v2, layer});
  }

  vertices = std::move(new_vertices);
  edges = std::move(new_edges);
}